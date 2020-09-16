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
 * TumblingWindowJoin.cpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "TumblingWindowJoin.hpp"
#include "../usecases/NQ8WS.hpp"

#include <unistd.h> // includes pthreads ?
#include <mpi.h> // helps to determine the first window bounds

using nexmark_new_sellers_ws::TumblingWindowJoin;
using nexmark_new_sellers_ws::OriginalTumblingWindowJoin;

TumblingWindowJoin::TumblingWindowJoin(int tag, int rank, int worldSize, int window_duration) :
OriginalTumblingWindowJoin(tag, rank, worldSize, window_duration){}

void TumblingWindowJoin::streamProcess(int channel) {

    // global aggregation operator
    if (rank == 0) {

        // init value depends on thread
		int expected_msgid = channel % worldSize;
        
        list<Message*>* tmpMessages = new list<Message*>();

        while (ALIVE) {

            updateInputMessages(tmpMessages, channel);
            sortMessages(tmpMessages); // new

			// new condition
            while (!tmpMessages->empty() && isMessageExpected(tmpMessages->front(), expected_msgid)) {

                Message* inMessage = tmpMessages->front();
                
                try
                {
                    OriginalTumblingWindowJoin::processInputMessage(inMessage);
                }
                catch(const char* e)
                {
                    std::cerr << e << '\n';
                }
                
                tmpMessages->pop_front();
                delete inMessage;

                updateExpectedValues(expected_msgid); // new
            }
        }

        delete tmpMessages;
    }
}

void TumblingWindowJoin::sortMessages(list<Message*>* tmpMessages){

	tmpMessages->sort(
		[](Message*const & a, Message*const & b){
		int a_msgid = Serialization::read_back<int>(a);
		int b_msgid = Serialization::read_back<int>(b);
		
        // smallest msgid first
		return (a_msgid < b_msgid);
	});
}

bool TumblingWindowJoin::isMessageExpected(Message*const inMessage, const int expected_msgid){
	int msgid = Serialization::read_back<int>(inMessage);

	return (msgid == expected_msgid);
}

void TumblingWindowJoin::updateExpectedValues(int & expected_msgid){
	// with one dataflow instance we expect each processing thread (2)
    // to process mgsids 0, 1, 2, ... once

    // with two dataflow instances, we expect each processing thread (4)
    // to process msgids from their instance (odds or even numbers) once

	expected_msgid += worldSize;
}