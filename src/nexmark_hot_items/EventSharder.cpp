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
 *  Created on: June 4, 2020
 *      Author: damien.tassetti
 */

#include "EventSharder.hpp"
#include "../usecases/NQ5.hpp"
#include "../serialization/Serialization.hpp"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits>
#include <mpi.h>

using nexmark_hot_items::EventSharder;
using nexmark_gen::Bid;
using nexmark_gen::Event;
using nexmark_gen::EventType;

EventSharder::EventSharder(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize),
msgid_max_wid(0),
msgid_min_wid(numeric_limits<int>::max()){}

void EventSharder::streamProcess(int channel) {

    list<Message*>* tmpMessages = new list<Message*>();

    while (ALIVE) {

        fetchInputMessages(tmpMessages, channel);

        while (!tmpMessages->empty()) {

            Message* inMessage = tmpMessages->front();
            processInputMessage(inMessage);
            tmpMessages->pop_front();
            delete inMessage;
        }

        // tmpMessages->clear();
    }

    delete tmpMessages;
}

void EventSharder::processInputMessage(Message*const inMessage){
    int message_id = Serialization::unwrap<int>(inMessage);

    ShardedMessage shards;
    shardEvents(shards, inMessage);
    send(shards, message_id);
}

void EventSharder::fetchInputMessages(list<Message*>* tmpMessages, const int channel){
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

void EventSharder::shardEvents(ShardedMessage& shards, Message*const inMessage){
    int nof_bids = inMessage->size / sizeof(Bid);

    const int window_duration = nexmark::NQ5::window_duration;
    const int window_shift = nexmark::NQ5::window_shift;

	for (size_t i = 0; i < nof_bids; i++)
	{
		const Bid& bid = Serialization::read_front<Bid>(inMessage, sizeof(Bid) * i);
        const int max_wid = bid.event_time / window_shift;
        const int min_wid = max_wid - min(max_wid, window_duration / window_shift - 1);

        if (max_wid > msgid_max_wid) msgid_max_wid = max_wid;
        if (min_wid < msgid_min_wid) msgid_min_wid = min_wid;

        for (size_t j = min_wid; j <= max_wid; j++)
        {
            shards[j].push_back(bid); // copy bid
        }
	}
}


// sends each message to the corresponding rank
void EventSharder::send(ShardedMessage &shards, const int message_id){

    for (auto it = shards.begin(); it != shards.end(); ++it)
    {
        assert(worldSize > 0);
        const int wid = (*it).first;
        const int targeted_rank = wid % worldSize; // first rank gets windows 0, ws, 2ws, ...
        const vector<Bid> & events = (*it).second;

        send(events, targeted_rank, wid, message_id); // channel = targeted rank
    }

    // reset min & max wids
    msgid_min_wid = numeric_limits<int>::max();
    msgid_max_wid = 0;

}

// we just pass the address of the message we wish to send, no modification intended
void EventSharder::send(const vector<Bid> & events, const int channel, const int wid, const int message_id){

    Message* outMessage = new Message(sizeof(Bid) * events.size() + sizeof(int) + sizeof(int) + 2*sizeof(int));
    for (size_t i = 0; i < events.size(); i++)
    {
        Serialization::wrap<Bid>(events[i], outMessage);
    }

    Serialization::wrap<int>(msgid_min_wid, outMessage);
    Serialization::wrap<int>(msgid_max_wid, outMessage); // helps to make sure we'll get messages in order
    Serialization::wrap<int>(wid, outMessage);
    Serialization::wrap<int>(message_id, outMessage);

    send(outMessage, channel);
}

void EventSharder::send(Message*const message, const int channel){
    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(message);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}