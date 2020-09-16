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
 * BidFilter.cpp
 *
 *  Created on: June 4, 2020
 *      Author: damien.tassetti
 */

#include "BidFilter.hpp"
#include "../serialization/Serialization.hpp"

#include <string.h>
#include <mpi.h>

using nexmark_hot_items::BidFilter;
using nexmark_gen::Bid;
using nexmark_gen::Event;
using nexmark_gen::EventType;

BidFilter::BidFilter(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize) {}

void BidFilter::streamProcess(int channel) {

    list<Message*>* tmpMessages = new list<Message*>();

    while (ALIVE) {

        fetchInputMessages(tmpMessages, channel);

        while (!tmpMessages->empty()) {

            Message* inMessage = tmpMessages->front();
            processInputMessage(inMessage);
            tmpMessages->pop_front();
            delete inMessage;
        }
    }

    delete tmpMessages;
}

void BidFilter::processInputMessage(Message*const message){
    int message_id = Serialization::unwrap<int>(message);
    
    Message* outMessage;
    vector<Bid> bids = filterBids(message);
    getNextMessage(outMessage, bids);

    Serialization::wrap<int>(message_id, outMessage); // id doesn't change

    send(outMessage);
}

void BidFilter::fetchInputMessages(list<Message*>* tmpMessages, const int channel){
	pthread_mutex_lock(&listenerMutexes[channel]);

	while (inMessages[channel].empty())
		pthread_cond_wait(&listenerCondVars[channel],
				&listenerMutexes[channel]);

	while (!inMessages[channel].empty()) {
		tmpMessages->push_back(inMessages[channel].front());
		inMessages[channel].pop_front();
	}

	pthread_mutex_unlock(&listenerMutexes[channel]);
}

Message* BidFilter::initMessage(size_t capacity) {
    return new Message(capacity);
}

void BidFilter::getNextMessage(Message*& outMessage, vector<Bid> & bids) {
    outMessage = initMessage(sizeof(Bid) * bids.size() + sizeof(int));

    for(size_t i = 0; i < bids.size(); i++){
        Serialization::wrap<Bid>(bids[i], outMessage);
    }
}

vector<Bid> BidFilter::filterBids(Message*const inMessage){

	unsigned int nof_events = inMessage->size / sizeof(Event);
    vector<Bid> bids;
    
	for(size_t i = 0; i < nof_events ; i++){
		const Event& e = Serialization::read_front<Event>(inMessage, sizeof(Event) * i);
		switch (e.event_type)
		{
		case EventType::BidType:
            bids.push_back(e.bid);
			break;
		
		default:
			// do nothing, try the next event
			break;
		}
	}
    return bids;
}

void BidFilter::send(Message*const message){

    for (size_t operatorIndex = 0; operatorIndex < next.size(); operatorIndex++) {

        // destination is next operator, on the same rank (no sharding)
        int idx = operatorIndex * worldSize + rank;
        send(message, idx);
    }
}

void BidFilter::send(Message*const message, const int channel){
    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(message);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}
