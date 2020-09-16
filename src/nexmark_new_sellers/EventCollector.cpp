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
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "EventCollector.hpp"
#include "../serialization/Serialization.hpp"

#include <string.h>
#include <mpi.h> // includes pthreads ?

using nexmark_new_sellers::EventCollector;
using nexmark_new_sellers::JoinOutput;

EventCollector::EventCollector(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize) {
    setSanityCheckOuput("NQ8.tsv");
}

void EventCollector::setSanityCheckOuput(string filename){
    S_CHECK(
        try
        {
            if (rank == 0){
                if (datafile.is_open()){
                    datafile.close();
                }
                
                datafile.open(filename);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
    );
}

void EventCollector::streamProcess(int channel) {

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

void EventCollector::processInputMessage(Message*const inMessage) {
    int window_id = Serialization::unwrap<int>(inMessage);
    int timestamp = Serialization::unwrap<int>(inMessage);
    
    vector<JoinOutput> aggregates = getAggregates(inMessage);

    // cout << "WID #" << window_id << '\n';
    // for(int i = 0; i < std::min((size_t)5, aggregates.size()); i++){
    //     cout << '\t' << aggregates[i].seller_name << " " << aggregates[i].reserve_price << '\n';
    // }
    // cout << '\n';

    double latency = MPI_Wtime() - timestamp;

    // no need for mutex because there is only one thread between the join and the collector
    avg_latency = ( avg_latency * window_id + latency ) / (window_id + 1); // window_id starts from 0 then to n-1 the number of windows processed
    cout << "NQ8 (" << baseline << "), " << worldSize << ", " << window_id << ", " << latency << ", " << avg_latency << '\n';

    for (size_t i = 0; i < aggregates.size(); i++)
    {
        S_CHECK(
            datafile << window_id << '\t' 
            << aggregates[i].seller_id << '\t'
            << aggregates[i].seller_name << '\t'
            << aggregates[i].auction_id << '\t'
            << aggregates[i].reserve_price << endl;
        );
    }
}

void EventCollector::updateInputMessages(list<Message*>*const tmpMessages, const int channel){
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

vector<JoinOutput> EventCollector::getAggregates(Message*const inMessage){
    // do not delete the input message here

	unsigned int nof_events = inMessage->size / sizeof(JoinOutput);
    vector<JoinOutput> aggregates;
    
	for(size_t i = 0; i < nof_events ; i++){
		const JoinOutput& e = Serialization::read_front<JoinOutput>(inMessage, sizeof(JoinOutput) * i);
        aggregates.push_back(e);
	}
    return aggregates;
}
