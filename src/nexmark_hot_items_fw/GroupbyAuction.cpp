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
 * GroupbyAuction.cpp
 *
 *  Created on: April 23, 2020
 *      Author: damien.tassetti
 */

#include "GroupbyAuction.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../usecases/NQ5FW.hpp"
#include "../serialization/Serialization.hpp"

#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>


using nexmark_hot_items_fw::GroupbyAuction;
using nexmark_gen::Bid;
using std::min;

GroupbyAuction::GroupbyAuction(int tag, int rank, int worldSize) :
		Vertex(tag, rank, worldSize) {
	pthread_mutex_init(&mutex, NULL);
}

GroupbyAuction::~GroupbyAuction() {
}

void GroupbyAuction::batchProcess() {
}

void GroupbyAuction::streamProcess(int channel) {

	Message* inMessage;
	list<Message*>* tmpMessages = new list<Message*>();
	const int window_duration = nexmark::NQ5FW::window_duration;

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

			WrapperUnit wu = Serialization::unwrap<WrapperUnit>(inMessage);
			int wid = wu.window_start_time; // TODO : check numeric limits

			pthread_mutex_lock(&mutex); // do not update structure and read it at the same time
			update(wid, inMessage);

			completeness[wid] += wu.completeness_tag_numerator; // every rank will send an is_complete message once

			// the first windows will never be complete
			if (completeness[wid] == wu.completeness_tag_denominator){
				send(wid);
				windows.erase(wid);
				window_ends.erase(wid);
				completeness.erase(wid);
			}
			pthread_mutex_unlock(&mutex);

			delete inMessage; // delete message from incoming queue

		}

		tmpMessages->clear();
	}

	delete tmpMessages;
}

void GroupbyAuction::update(int wid, Message*const inMessage){

	size_t nof_bids = inMessage->size / sizeof(Bid);
	for(size_t i = 0; i < nof_bids; i++){
		const Bid& bid = Serialization::read_front<Bid>(inMessage, sizeof(Bid) * i);
		windows[wid][bid.auction_id]++;

		if (bid.event_time > window_ends[wid]) window_ends[wid] = bid.event_time;
	}
}


void GroupbyAuction::send(int wid){

	Message* outMessage = new Message(windows[wid].size() * (sizeof(int) + sizeof(size_t)) + sizeof(WrapperUnit));
	
	for(auto it = windows[wid].begin(); it != windows[wid].end(); ++it){
		Serialization::wrap<int>((*it).first, outMessage); // auction_id
		Serialization::wrap<size_t>((*it).second, outMessage); // nof bids for the given auction
	}

	WrapperUnit wu;
	wu.window_start_time = wid; // the rest doesn't matter
	Serialization::wrap<WrapperUnit>(wu, outMessage);

	int channel = 0; // send to the global aggregator on rank 0
	pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(outMessage);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}