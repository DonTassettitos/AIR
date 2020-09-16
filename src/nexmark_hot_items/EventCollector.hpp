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
 * EventCollector.hpp
 *
 *  Created on: June 4, 2020
 *      Author: damien.tassetti
 */

#pragma once
#include "../dataflow/Vertex.hpp"
#include <fstream>

namespace nexmark_hot_items {

typedef vector<pair<int, size_t>> vec;

class EventCollector: public Vertex {

	public:
	EventCollector(int tag, int rank, int worldSize);
	~EventCollector();
	void streamProcess(int channel);

	protected:
	virtual void fetchInputMessages(list<Message*>* tmpMessages, const int channel);
	virtual void processInputMessage(Message*const message);
	virtual void display(int wid, vec & auctions);
	virtual vec getAuctions(const Message* const inMessage);
	virtual void sortAuctionsByCount(vec & auctions);
	virtual void filterAuctionsWithMaxCount(vec & auctions);

	private:
	int message_id;
	pthread_mutex_t mutex;
	double avg_latency;

	std::ofstream datafile; // for sanity checks
};
};