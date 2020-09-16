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
 * TumblingWindowJoin.hpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#pragma once
#include "../dataflow/Vertex.hpp"
#include "PODTypesExtension.hpp"

#include <map>
#include <mpi.h> // for threads

namespace nexmark_new_sellers {
using nexmark_new_sellers::JoinInputType;
using nexmark_new_sellers::JoinOutput;

/**
 * Vertex input : JoinInput (PersonSelection, AuctionSelection)
 * Vertex output : JoinOutput (person id and name, as well as corresponding auction reserve price)
 * 
 * Parallel : yes (runs 2 threads per dataflow, one from the person selector and the other from the auction selector)
 * Distributed : no (this is a global aggregation operation, we need the events from all instances to make sure the final result is correct)
 **/
class TumblingWindowJoin : public Vertex {

    public:
    TumblingWindowJoin(int tag, int rank, int worldSize, int window_duration);
    void streamProcess(int channel);

    protected:
    virtual void updateInputMessages(list<Message*>*const tmpMessages, const int channel);
    virtual void processInputMessage(Message*const inMessage);
    virtual void processAuctionMessage(Message*const inMessage, const int window_id);
    virtual void processPersonMessage(Message*const inMessage, const int window_id);
    virtual void processAuctionEvent(AuctionSelection& auction, const int window_id);
    virtual void processPersonEvent(PersonSelection& person, const int window_id);
    virtual bool isCurrentWindowComplete();
    virtual void computeAggregates();
    virtual Message*const buildOutputMessage(); // for first window
    virtual Message*const initMessage(size_t message_size);
    virtual JoinOutput combine(AuctionSelection& auction, PersonSelection& person);
    virtual void send(Message*const message);
    virtual void send(Message*const message, const int channel);
    typedef struct {
        vector<JoinOutput> aggregates;
        vector<AuctionSelection> auctions;
        map<int, PersonSelection> persons;
        int completeness = 0;
        int timestamp_end = 0;
    } WindowInformation;

    int window_duration;
    map<int, WindowInformation> windows;

    private:
    pthread_mutex_t mtx;
    pthread_mutex_t auctions_mtx;
    pthread_mutex_t persons_mtx;
};
};
