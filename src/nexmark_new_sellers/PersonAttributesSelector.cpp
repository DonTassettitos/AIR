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
 * PersonAttributesSelector.cpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "PersonAttributesSelector.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../serialization/Serialization.hpp"

#include <string.h>
#include <unistd.h> // includes pthreads ?

using nexmark_gen::Person;
using nexmark_new_sellers::PersonSelection;
using nexmark_new_sellers::PersonAttributesSelector;

PersonAttributesSelector::PersonAttributesSelector(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize) {
}

void PersonAttributesSelector::streamProcess(int channel) {

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

void PersonAttributesSelector::processInputMessage(Message*const inMessage) {
    int message_id = Serialization::unwrap<int>(inMessage);
    int timestamp = Serialization::unwrap<int>(inMessage);
    
    Message* outMessage;
    vector<PersonSelection> selection = selectAttributes(inMessage);
    getNextMessage(outMessage, selection);

    Serialization::wrap<JoinInputType>(JoinInputType::PersonSelectionType, outMessage);
    Serialization::wrap<int>(timestamp, outMessage);
    Serialization::wrap<int>(message_id, outMessage); // id doesn't change

    // cout << "[person filter] sent msgid: " << message_id << " (thread " << pthread_self() << ")" << endl; 
    send(outMessage);
}

void PersonAttributesSelector::updateInputMessages(list<Message*>*const tmpMessages, const int channel){
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

Message* PersonAttributesSelector::initMessage(size_t capacity) {
    return new Message(capacity + sizeof(JoinInputType) + 2*sizeof(int));
}

void PersonAttributesSelector::getNextMessage(Message*& outMessage, vector<PersonSelection> & selection) {
    outMessage = initMessage(sizeof(PersonSelection) * selection.size());

    for(size_t i = 0; i < selection.size(); i++){
        Serialization::wrap<PersonSelection>(selection[i], outMessage);
    }
}

vector<PersonSelection> PersonAttributesSelector::selectAttributes(Message*const inMessage){
    // do not delete the input message here

	unsigned int nof_events = inMessage->size / sizeof(Person);
    vector<PersonSelection> selection;
    
	for(size_t i = 0; i < nof_events ; i++){
		const Person& e = Serialization::read_front<Person>(inMessage, sizeof(Person) * i);

        PersonSelection p;
        p.id = e.id;
        strcpy(p.name, e.name);

        // no need to copy the string now as long as the message is not deleted
        // or message buffer cleared, but I get error: invalid array assignment
        // if I don't

		selection.push_back(p);
	}
    return selection;
}

void PersonAttributesSelector::send(Message*const message){

    // only one operator next, on the same dataflow
    send(message, 0);
}

void PersonAttributesSelector::send(Message*const message, const int channel){
    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(message);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}
