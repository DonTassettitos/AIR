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
 *  Created on: April 16, 2020
 *      Author: damien.tassetti
 */

#include "BidFilter.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../serialization/Serialization.hpp"

#include <string.h>
#include <mpi.h>

using nexmark_hot_items_ws::BidFilter;
using nexmark_gen::Bid;
using nexmark_gen::Event;
using nexmark_gen::EventType;

BidFilter::BidFilter(int tag, int rank, int worldSize) :
		Vertex(tag, rank, worldSize) {
}

BidFilter::~BidFilter() {
}

void BidFilter::batchProcess() {
}

void BidFilter::streamProcess(int channel) {

    Message* inMessage;
    list<Message*>* tmpMessages = new list<Message*>();

    while (ALIVE) {

        pthread_mutex_lock(&listenerMutexes[channel]);

        while (inMessages[channel].empty())
            pthread_cond_wait(&listenerCondVars[channel],
                    &listenerMutexes[channel]);

        while (!inMessages[channel].empty()) {
            inMessage = inMessages[channel].front();
            inMessages[channel].pop_front();
            tmpMessages->push_back(inMessage);
        }

        pthread_mutex_unlock(&listenerMutexes[channel]);

        while (!tmpMessages->empty()) {

            inMessage = tmpMessages->front();
            tmpMessages->pop_front();

            int message_id = Serialization::unwrap<int>(inMessage);
            
            Message* outMessage;
            vector<Bid> bids = filterBids(inMessage);
            getNextMessage(outMessage, bids);

            Serialization::wrap<int>(message_id, outMessage); // id doesn't change

            send(outMessage);

            delete inMessage; // delete message from incoming queue
        }

        tmpMessages->clear();
    }

    delete tmpMessages;
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

// we wish to modify where outMessage points to, but just read inMessage
vector<Bid> BidFilter::filterBids(Message*const inMessage){
    // do not delete the input message here

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

// we just pass the address of the message we wish to send, no modification intended
void BidFilter::send(Message*const message){

    for (size_t operatorIndex = 0; operatorIndex < next.size(); operatorIndex++) {

        // messages are not sent to another dataflow (no sharding)
        int idx = operatorIndex * worldSize + rank;

        pthread_mutex_lock(&senderMutexes[idx]);
        outMessages[idx].push_back(message);
        pthread_cond_signal(&senderCondVars[idx]);
        pthread_mutex_unlock(&senderMutexes[idx]);
    }
}
