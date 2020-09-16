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
 *  Created on: June 4, 2020
 *      Author: damien.tassetti
 */

#include "GroupbyAuction.hpp"
#include "../usecases/NQ5.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../serialization/Serialization.hpp"

#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cmath>


using nexmark_hot_items::GroupbyAuction;
using nexmark_gen::Bid;
using std::min;

GroupbyAuction::GroupbyAuction(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize) {
	pthread_mutex_init(&windows_mutex, NULL);
	pthread_mutex_init(&completeness_mutex, NULL);
}

void GroupbyAuction::streamProcess(int channel) {

	// one instance per thread (i.e. for one rank, we have rank threads)
	list<Message*>* tmpMessages = new list<Message*>();
	
	while (ALIVE) {

		fetchInputMessages(tmpMessages, channel);

		if (!tmpMessages->empty()){

			while (!tmpMessages->empty()) {

				Message* inMessage = tmpMessages->front();
				processInputMessage(inMessage);
				tmpMessages->pop_front();
				delete inMessage;
			}
		}
	}
	delete tmpMessages;
}

void GroupbyAuction::processInputMessage(Message*const message){
	int wid = Serialization::read_back<int>(message, sizeof(int));
	
	// count nof bids per auction for the given window data, and update current count
	int header_size = 4*sizeof(int);
	size_t nof_bids = (message->size - header_size) / sizeof(Bid);

	for(size_t i = 0; i < nof_bids; i++){
		const Bid& bid = Serialization::read_front<Bid>(message, sizeof(Bid) * i);

		pthread_mutex_lock(&windows_mutex);
		windows[wid][bid.auction_id]++;
		pthread_mutex_unlock(&windows_mutex);
	}

	// update completeness information
	pthread_mutex_lock(&completeness_mutex);
	completeness.emplace(wid, 0);
	completeness[wid]++;

	const int window_duration = nexmark::NQ5::window_duration;
	if (completeness[wid] == PER_SEC_MSG_COUNT * window_duration * worldSize){
		pthread_mutex_lock(&windows_mutex);
		send(wid);
		windows.erase(wid);
		completeness.erase(wid);
		pthread_mutex_unlock(&windows_mutex);
	}
	pthread_mutex_unlock(&completeness_mutex);
}

void GroupbyAuction::fetchInputMessages(list<Message*>* tmpMessages, const int channel){
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

void GroupbyAuction::send(int wid){

	// message = window id + list of counts per auction
	Message* outMessage = new Message(sizeof(int) + windows[wid].size() * (sizeof(int) + sizeof(size_t)));
	
	for(auto it = windows[wid].begin(); it != windows[wid].end(); ++it){
		Serialization::wrap<int>((*it).first, outMessage); // auction_id
		Serialization::wrap<size_t>((*it).second, outMessage); // nof bids for the given auction
	}

	Serialization::wrap<int>(wid, outMessage);

	// send to the global aggregator on rank 0
	send(outMessage, 0);
}

void GroupbyAuction::send(Message*const message, const int channel){
    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(message);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}