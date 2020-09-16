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
 *  Created on: April 17, 2020
 *      Author: damien.tassetti
 */

#include "GroupbyAuction.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../usecases/NQ5WS.hpp"
#include "../serialization/Serialization.hpp"

#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cmath>


using nexmark_hot_items_ws::GroupbyAuction;
using nexmark_gen::Bid;
using std::min;

GroupbyAuction::GroupbyAuction(int tag, int rank, int worldSize) :
		OriginalGroupbyAuction(tag, rank, worldSize) {}

void GroupbyAuction::streamProcess(int channel) {

	// one instance per thread (i.e. for one rank, we have rank threads)
	Message* inMessage;
	list<Message*>* tmpMessages = new list<Message*>();
	int expected_msgid;
	int expected_wid;
	bool is_first_time = true;
	bool is_expected_msgid_being_processed = false;

	while (ALIVE) {

		// update inbox
		OriginalGroupbyAuction::fetchInputMessages(tmpMessages, channel);

		if (!tmpMessages->empty()){

			// process events in order (makes sure windows really are finished
			// when we test if they are complete)
			sortMessages(tmpMessages);

			// the first time the thread arrives here, we don't know what rank it comes from
			// this method reads the first message id to determine that, and the thread will
			// continue to process messages from this rank
			initExpectedMID(tmpMessages->front(), expected_msgid, is_first_time);
			
			// once we know the msgid we start from, we can determine what wids we will be 
			// confronted to. The min and max wid values are stored int every messages and
			// are unique to each message id.
			initExpectedWID(tmpMessages->front(), expected_msgid, expected_wid, is_expected_msgid_being_processed);

			// as messages are sorted, process as many as we possibly can
			// if the msgid or wid is not expected, wait for other messages to come in
			// that much is also true if there are no more messages to read from the list
			while (!tmpMessages->empty() && isMessageExpected(tmpMessages->front(), expected_msgid, expected_wid)) {

				// inMessage = header + list of bids
				// with msgid = sharder rank + n * worldSize (odd or even if worldSize == 2)
				// note that msgid is not global, it is dependent on the current thread's origin (i.e. event sharder)
				// and wid depends on current operator rank (odd or even is worldSize == 2)
				inMessage = tmpMessages->front();
				tmpMessages->pop_front(); 

				// header = msgid + wid + max_wid + min_wid
				int wid = Serialization::read_back<int>(inMessage, sizeof(int));
				int min_wid = Serialization::read_back<int>(inMessage, sizeof(int)*3);

				OriginalGroupbyAuction::processInputMessage(inMessage);

				// once the current message has been processed, we need to know if
				// we can process the next one, i.e. we need to determine what msgid
				// and wid values we expect to find in the next message.
				// 
				// note : we expect different values for every thread.
				updateExpectedValues(inMessage, tmpMessages, expected_msgid, expected_wid, is_expected_msgid_being_processed);

				delete inMessage; // delete message from incoming queue
			}
		}
	}

	delete tmpMessages;
}

void GroupbyAuction::updateExpectedValues(Message*const currentMessage, list<Message*>* & messageList, int & expected_msgid, int & expected_wid, bool & is_expected_msgid_being_processed){
	// we expect the next msgid to be different from the current one
	// only if we already processed all the wids of this msgid that
	// can be processed on this rank and on this thread.
	int msgid_max_wid = Serialization::read_back<int>(currentMessage, sizeof(int)*2);

	// if we have to keep the current msgid, we know the next wid is :
	int next_wid = expected_wid + worldSize;

	// if (rank == 1) cout << "trying to update the expected wid " << expected_wid << " , thread " << pthread_self() << " tries value " << next_wid << endl;

	if (next_wid <= msgid_max_wid){
		// we know the event sharder generated at least that many messages
		// so we can expect to process a message with this wid before changing
		// the msgid
		expected_wid = next_wid;

		// however, if the message list if empty, then the thread will go
		// outside the loop and try to get new messages. If it happends to
		// get a new message with the same msgid, the initExpectedWID()
		// method will reinitialize the expected wid value, as if the first 
		// few messages weren't processed already. That's why we must make sure
		// this method isn't called

		is_expected_msgid_being_processed = true;

		// if (rank == 1) cout << "and it is accepted because " << expected_wid << " < " << msgid_max_wid << endl;

	} else {
		// the event sharder didn't generate that many messages
		// so we have to change msgid to see if there are other 
		// messages pertaining to this rank's wids.
		expected_msgid = expected_msgid + worldSize;
		
		// expected wid will depend on how many messages the 
		// event sharder generated for the next msgid. Which
		// means we can't tell what value to expect, unless
		// we can read a message with this msgid (I tried to 
		// calculate min_wid and max_wid knowing event times
		// and using the sharding formula from the previous
		// operator, however I couldn't get it to work, there
		// was always a time when min_wid was too small or too
		// big, and when it wasn't min_wid, it was max_wid, so
		// it might be possible to make it work, but for know
		// we'll use another method)

		// we'll have to make sure the current thread will 
		// get us the correct value for the expected wid.
		// For that, we tell the thread we have completely
		// processed the previous msgid, and that we have 
		// new one now :
		is_expected_msgid_being_processed = false;

		if (messageList->size() > 0) {
			// we have other messages to process in the waiting list
			// so if the next one has the expected msgid, we can
			// determine the next expected value for wid
			//
			// note : this is equivalent to checking is the msgid of
			// the front message is correct and update the wid if that
			// is the case, which is equivalent to calling the method
			// initExpectedWID() (initial front message has been popped)

			initExpectedWID(messageList->front(), expected_msgid, expected_wid, is_expected_msgid_being_processed);

			// if the front message doesn't have the correct msgid value, 
			// then the tread will go outside the while loop and wait for 
			// a message with the correct msgid

			// no need to check every message of the list because they should
			// be sorted by msgid, only the first is important

			// if (rank == 1) cout << "but it is too big so we take a look at the waiting list, looking for msgid " << expected_msgid << " and wid is now " << expected_wid << endl;

		} else {
			// if there is no more messages in the waiting list
			// then the thread will go outside the while loop and 
			// load new messages. If there are new messages, then 
			// the initExpectedWID() method will be called and 
			// if the msgid is the expected value, then the wid
			// will be updated
			//
			// conclusion : no need to do anything here !

			// if (rank == 1) cout << "but it is too big so we wait for the next message with msgid " << expected_wid << endl;

		}
	}
}


void GroupbyAuction::sortMessages(list<Message*>* tmpMessages){

	// if (rank == 1) cout <<endl << "waiting for new messages ..." << endl;

	// does what is says :
	// primary sort criterion is msgid
	// secondary sort criterion is wid
	// which ouputs something like (0; 0), (0; 3), (1; 2), ...
	// with (msgid, wid) the template of the above tuples 
	tmpMessages->sort(
		[](Message*const & a, Message*const & b){
		int a_msgid = Serialization::read_back<int>(a);
		int b_msgid = Serialization::read_back<int>(b);
		int a_wid = Serialization::read_back<int>(a, sizeof(int));
		int b_wid = Serialization::read_back<int>(b, sizeof(int));
		
		return (a_msgid < b_msgid) || (a_msgid == b_msgid && a_wid < b_wid);
	});

	// display
	// for (auto const & message : *tmpMessages){
	// 	int msgid = Serialization::read_back<int>(message);
	// 	int wid = Serialization::read_back<int>(message, sizeof(int));
	// 	int max_wid = Serialization::read_back<int>(message, sizeof(int) * 2);
	// 	int min_wid = Serialization::read_back<int>(message, sizeof(int) * 3);
	// 	// if (rank == 1) cout << "msgid : " << msgid << " wid : " << wid << " min_wid : " << min_wid << " max_wid : " << max_wid << endl;
	// }
}

void GroupbyAuction::initExpectedMID(Message*const inMessage, int & expected_msgid, bool & is_first_time){
	// if msgid is 1, then we know that the messages will all come from rank 1
	// which means this thread will process all wids for msgid 1 + n * worldSize
	// some wids are in common, so be careful to use a mutex when you aggregate
	// for a given wid.
	if (is_first_time) {
		int msgid = Serialization::read_back<int>(inMessage);
		expected_msgid = msgid;
		is_first_time = false;

		// display
		// if (rank == 1) cout << "initialized expected msgid with value : " << msgid << endl;
	}
}


void GroupbyAuction::initExpectedWID(Message*const inMessage, const int expected_msgid, int & expected_wid, bool & is_expected_msgid_being_processed){
	int msgid = Serialization::read_back<int>(inMessage);
	
	// if the expected msgid corresponds to the given msgid, but its processing
	// is already under way, then the expected wid mustn't change. We have to wait
	// for a message id with the corresponding wid.
	if (msgid == expected_msgid && !is_expected_msgid_being_processed) {
		is_expected_msgid_being_processed = true; // we started processing this msgid
		int min_wid = Serialization::read_back<int>(inMessage, sizeof(int)*3); // header = msgid + wid + max_wid + min_wid
		int max_wid = Serialization::read_back<int>(inMessage, sizeof(int)*2); // header = msgid + wid + max_wid + min_wid
		
		int starting_rank = (min_wid % worldSize); // rank expected to receive & process min_wid
		if (rank >= starting_rank) {
			expected_wid = min_wid + (rank - starting_rank);
		} else { // rank < starting_rank
			expected_wid = min_wid + worldSize - ( starting_rank - rank);
		}

		// if (rank == 1) cout << "initialized expected wid with value : " << expected_wid << endl;
	} else {
		// do not change the expected wid
		// we have to wait for the message containing the expected wid
		// as the msgid is wrong, the current front message will not be processed by the following methods

		// so : do nothing !
	}
}

bool GroupbyAuction::isMessageExpected(Message*const inMessage, const int expected_msgid, const int expected_wid){
	int msgid = Serialization::read_back<int>(inMessage);
	int wid = Serialization::read_back<int>(inMessage, sizeof(int));

	// if (msgid == expected_msgid && wid == expected_wid) {
	// 	// if (rank == 1) cout << "message with msgid " << msgid << " and wid " << wid << " IS expected" << endl;
	// } else {
	// 	// if (rank == 1) cout << "message with msgid " << msgid << " and wid " << wid << " is NOT expected" << endl;
	// }

	return (msgid == expected_msgid && wid == expected_wid);
}