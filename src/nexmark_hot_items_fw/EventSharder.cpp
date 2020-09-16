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
 * EventSharder.cpp
 *
 *  Created on: April 23, 2020
 *      Author: damien.tassetti
 */

#include "EventSharder.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../usecases/NQ5FW.hpp"
#include "../serialization/Serialization.hpp"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>

using nexmark_hot_items_fw::EventSharder;
using nexmark_gen::Bid;
using nexmark_gen::Event;
using nexmark_gen::EventType;

EventSharder::EventSharder(int tag, int rank, int worldSize) :
		Vertex(tag, rank, worldSize) {
}

EventSharder::~EventSharder() {
}

void EventSharder::batchProcess() {
}

void EventSharder::streamProcess(int channel) {

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
            
            // process message
            int message_id = Serialization::unwrap<int>(inMessage); // not used

            WrapperUnit wu = Serialization::unwrap<WrapperUnit>(inMessage); // not used
			
            ShardedMessage shards; // resets every time
            shardEvents(shards, inMessage);
            send(shards);

            // update ids
            delete inMessage; // delete message from incoming queue

        }

        tmpMessages->clear();
    }

    delete tmpMessages;
}

void EventSharder::shardEvents(ShardedMessage& shards, Message*const inMessage){
    int nof_bids = inMessage->size / sizeof(Bid);

    const int window_duration = nexmark::NQ5FW::window_duration;
    const int window_shift = nexmark::NQ5FW::window_shift;

	for (size_t i = 0; i < nof_bids; i++)
	{
		const Bid& bid = Serialization::read_front<Bid>(inMessage, sizeof(Bid) * i);
        const int max_wid = bid.event_time / window_shift;
        const int min_wid = max_wid - min(max_wid, window_duration / window_shift - 1);

        for (size_t j = min_wid; j <= max_wid; j++)
        {
            shards[j].push_back(bid); // copy bid
        }
	}
}


// sends each message to the corresponding rank
void EventSharder::send(ShardedMessage &shards){
    const int window_shift = nexmark::NQ5FW::window_shift;
    const int window_duration = nexmark::NQ5FW::window_duration;

    for (auto it = shards.begin(); it != shards.end(); ++it)
    {
        assert(worldSize > 0);
        const int wid = (*it).first;
        const int targeted_rank = wid % worldSize; // first rank gets windows 0, ws, 2ws, ...
        const vector<Bid> & events = (*it).second;

        // checks if window is complete based on last event time of the message
        // e.g. last event time is 3600 then 0 * 60 + 3600 <= 3600 so the first window is complete
        //      but if last event time is 3600 then 1 * 60 + 3600 > 3600 so the second window is not complete

        for (size_t operatorIndex = 0; operatorIndex < next.size(); operatorIndex++) {
            const int channel = operatorIndex * worldSize + targeted_rank;
            send(events, channel, wid);
        }
    }
}

// we just pass the address of the message we wish to send, no modification intended
void EventSharder::send(const vector<Bid> & events, const int channel, int wid){

    Message* outMessage; getNextMessage(outMessage, events);
    
    WrapperUnit wu;
    wu.window_start_time = wid;
    wu.completeness_tag_numerator = 1;
    wu.completeness_tag_denominator = nexmark::NQ5FW::window_duration * PER_SEC_MSG_COUNT * worldSize;
    Serialization::wrap<WrapperUnit>(wu, outMessage);

    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(outMessage);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}

void EventSharder::getNextMessage(Message*& outMessage, const vector<Bid> & events) {
    outMessage = initMessage(sizeof(Bid) * events.size() + sizeof(WrapperUnit));

    for (size_t i = 0; i < events.size(); i++)
    {
        Serialization::wrap<Bid>(events[i], outMessage);
    }
}

Message* EventSharder::initMessage(size_t capacity){
    return new Message(capacity);
}