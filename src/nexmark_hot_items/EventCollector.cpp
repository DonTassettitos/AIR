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
 * EventCollector.cpp
 *
 *  Created on: June 4, 2020
 *      Author: damien.tassetti
 */

#include "EventCollector.hpp"
#include "../usecases/NQ5.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../serialization/Serialization.hpp"

#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

using nexmark_hot_items::EventCollector;
using nexmark_hot_items::vec;
using nexmark_gen::Bid;
using std::min;

EventCollector::EventCollector(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize),
message_id(0),
avg_latency(0.0) {	
	pthread_mutex_init(&mutex, NULL);

	S_CHECK(if (rank == 0){
		datafile.open("NQ5.tsv");
	})
}

EventCollector::~EventCollector(){
	datafile.close();
}

void EventCollector::streamProcess(int channel) {

	if (rank == 0) {

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
}

void EventCollector::processInputMessage(Message*const inMessage){
	int wid = Serialization::unwrap<int>(inMessage);

	vec auctions = getAuctions(inMessage);
	sortAuctionsByCount(auctions);
	filterAuctionsWithMaxCount(auctions);

	display(wid, auctions);
}

void EventCollector::fetchInputMessages(list<Message*>* tmpMessages, const int channel){
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

void EventCollector::filterAuctionsWithMaxCount(vec & auctions){
	// remove auctions for which nof_bids isn't the max
	vec::iterator iter = remove_if(auctions.begin(), auctions.end(), [&auctions](pair<int, size_t> & a){
		return a.second < auctions[0].second;
	});
	auctions.erase(iter, auctions.end());
}

void EventCollector::sortAuctionsByCount(vec & auctions){
	// more efficient than using a map because we only sort once
	sort(auctions.begin(), auctions.end(), [](pair<int, size_t> & a, pair<int, size_t> & b){
		return a.second > b.second;
	});
}

vec EventCollector::getAuctions(const Message* const message){

	size_t pair_size = sizeof(int) + sizeof(size_t);
	int nof_auctions = message->size / pair_size;

	vec auctions; auctions.reserve(nof_auctions);
	for (size_t i = 0; i < nof_auctions; i++)
	{
		const int auction_id = Serialization::read_front<int>(message, pair_size * i);
		const size_t nof_bids = Serialization::read_front<size_t>(message, pair_size * i + sizeof(int));
		auctions.push_back(pair<int, size_t>(auction_id, nof_bids));
	}

	return auctions;
}

void EventCollector::display(int wid, vec & auctions){
	// do not delete the message here

	// display result
	// cout << " #" << message_id << " WID: " << wid << " \n";
	// for (size_t i = 0; i < min(auctions.size(), (size_t)5); i++)
	// {
	// 	cout << auctions[i].first << " : " << auctions[i].second << "\n";
	// } cout << endl;

	// latency
	pthread_mutex_lock(&mutex);
	const int agg_shift = nexmark::NQ5::window_shift;
	const int agg_range = nexmark::NQ5::window_duration;

	double latency = MPI_Wtime() - (wid * agg_shift + agg_range);
	avg_latency = (avg_latency * message_id + latency ) / (message_id + 1);

	// cout << "(WID)" << wid << " " << message_id << ", " << latency << ", " << avg_latency << "\n";
	cout << "NQ5 (count), " << worldSize << ", " << wid << ", " << latency << ", " << avg_latency << "\n";

	
	for (size_t i = 0; i < auctions.size(); i++)
	{
		S_CHECK(datafile << wid << '\t' 
			<< auctions[i].first << '\t'
			<< auctions[i].second << endl;
		);
	}

	message_id++;
	pthread_mutex_unlock(&mutex);

}
