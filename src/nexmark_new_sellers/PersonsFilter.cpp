/**
 * Copyright (c) 2020 University of Luxembourg. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF LUXEMBOURG AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE UNIVERSITY OF LUXEMBOURG OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

/*
 * PersonsFilter.cpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "PersonsFilter.hpp"
#include "../serialization/Serialization.hpp"

#include <string.h>
#include <unistd.h> // includes pthreads ?

using nexmark_gen::Person;
using nexmark_gen::Event;
using nexmark_gen::EventType;
using nexmark_new_sellers::PersonsFilter;

PersonsFilter::PersonsFilter(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize) {
}

void PersonsFilter::streamProcess(int channel) {

    list<Message*>* tmpMessages = new list<Message*>();

    while (ALIVE) {

        updateInputMessages(tmpMessages, channel);

        while (!tmpMessages->empty()) {
            Message* inMessage = tmpMessages->front();
            processInputMessage(inMessage);
            tmpMessages->pop_front(); // detaches message pointer from list
            delete inMessage; // delete message
        }
    }

    delete tmpMessages;
}

void PersonsFilter::processInputMessage(Message*const inMessage) {
    int message_id = Serialization::unwrap<int>(inMessage);
    int timestamp = Serialization::unwrap<int>(inMessage);

    Message* outMessage;
    vector<Person> persons = filterPersons(inMessage);
    getNextMessage(outMessage, persons);

    Serialization::wrap<int>(timestamp, outMessage);
    Serialization::wrap<int>(message_id, outMessage); // id doesn't change

    send(outMessage);
}

void PersonsFilter::updateInputMessages(list<Message*>*const tmpMessages, const int channel){
    Message* inMessage;
    
    pthread_mutex_lock(&listenerMutexes[channel]);

    while (inMessages[channel].empty())
        pthread_cond_wait(&listenerCondVars[channel],
                &listenerMutexes[channel]);

    while (!inMessages[channel].empty()) {
        inMessage = inMessages[channel].front();
        tmpMessages->push_back(inMessage);
        inMessages[channel].pop_front();
    }

    pthread_mutex_unlock(&listenerMutexes[channel]);
}

Message* PersonsFilter::initMessage(size_t capacity) {
    return new Message(capacity + 2*sizeof(int));
}

void PersonsFilter::getNextMessage(Message*& outMessage, vector<Person> & persons) {
    outMessage = initMessage(sizeof(Person) * persons.size());

    for(size_t i = 0; i < persons.size(); i++){
        Serialization::wrap<Person>(persons[i], outMessage);
    }
}

vector<Person> PersonsFilter::filterPersons(Message*const inMessage){
    // do not delete the input message here

	unsigned int nof_events = inMessage->size / sizeof(Event);
    vector<Person> persons;
    
	for(size_t i = 0; i < nof_events ; i++){
		const Event& e = Serialization::read_front<Event>(inMessage, sizeof(Event) * i);
		switch (e.event_type)
		{
		case EventType::PersonType:
            persons.push_back(e.person);
			break;
		
		default:
			// do nothing, try the next event
			break;
		}
	}
    return persons;
}

// we just pass the address of the message we wish to send, no modification intended
void PersonsFilter::send(Message*const message){

    for (size_t operatorIndex = 0; operatorIndex < next.size(); operatorIndex++) {

        // messages are not sent to another dataflow (no sharding)
        int idx = operatorIndex * worldSize + rank;

        send(message, idx);
    }
}

void PersonsFilter::send(Message*const message, const int channel){
    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(message);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}
