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
 * AuctionAttributesSelector.cpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "AuctionAttributesSelector.hpp"
#include "../nexmark_gen/PODTypes.hpp"
#include "../serialization/Serialization.hpp"

#include <string.h>
#include <unistd.h> // includes pthreads ?
#include <iostream>

using nexmark_gen::Auction;
using nexmark_new_sellers::AuctionSelection;
using nexmark_new_sellers::AuctionAttributesSelector;
using std::cout;
using std::endl;

AuctionAttributesSelector::AuctionAttributesSelector(int tag, int rank, int worldSize) :
Vertex(tag, rank, worldSize) {
}

void AuctionAttributesSelector::streamProcess(int channel) {

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

void AuctionAttributesSelector::processInputMessage(Message*const inMessage) {
    int message_id = Serialization::unwrap<int>(inMessage);
    int timestamp = Serialization::unwrap<int>(inMessage);

    Message* outMessage;
    vector<AuctionSelection> selection = selectAttributes(inMessage);
    getNextMessage(outMessage, selection);

    Serialization::wrap<int>(JoinInputType::AuctionSelectionType, outMessage);
    Serialization::wrap<int>(timestamp, outMessage);
    Serialization::wrap<int>(message_id, outMessage); // id doesn't change

    // cout << "[auctions filter] sent msgid: " << message_id << " (thread " << pthread_self() << ")" << endl; 
    send(outMessage);
}

void AuctionAttributesSelector::updateInputMessages(list<Message*>*const tmpMessages, const int channel){
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

Message* AuctionAttributesSelector::initMessage(size_t capacity) {
    return new Message(capacity + sizeof(JoinInputType) + 2*sizeof(int));
}

void AuctionAttributesSelector::getNextMessage(Message*& outMessage, vector<AuctionSelection> & selection) {
    outMessage = initMessage(sizeof(AuctionSelection) * selection.size());

    for(size_t i = 0; i < selection.size(); i++){
        Serialization::wrap<AuctionSelection>(selection[i], outMessage);
    }
}

vector<AuctionSelection> AuctionAttributesSelector::selectAttributes(Message*const inMessage){
    // do not delete the input message here

	unsigned int nof_events = inMessage->size / sizeof(Auction);
    vector<AuctionSelection> selection;
    
	for(size_t i = 0; i < nof_events ; i++){
		const Auction& e = Serialization::read_front<Auction>(inMessage, sizeof(Auction) * i);

        AuctionSelection a;
        a.seller_id = e.seller_id;
        if (e.reserve_price_is_null){
            a.reserve_price = 0;
        } else {
            a.reserve_price = e.reserve_price;
        }
        a.id = e.id;

		selection.push_back(a);
	}
    return selection;
}

void AuctionAttributesSelector::send(Message*const message){

    // only one operator next, on the same dataflow
    send(message, 0);
}

void AuctionAttributesSelector::send(Message*const message, const int channel) {
    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(message);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}
