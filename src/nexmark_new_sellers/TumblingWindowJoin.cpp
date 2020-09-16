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

#include <unistd.h> // includes pthreads ?
#include <mpi.h> // helps to determine the first window bounds

using nexmark_new_sellers::TumblingWindowJoin;
using nexmark_new_sellers::AuctionSelection;
using nexmark_new_sellers::PersonSelection;
using nexmark_new_sellers::JoinInputType;
using nexmark_new_sellers::JoinOutput;

TumblingWindowJoin::TumblingWindowJoin(int tag, int rank, int worldSize, int window_duration) :
Vertex(tag, rank, worldSize),
window_duration(window_duration){
    pthread_mutex_init(&mtx, NULL);
    pthread_mutex_init(&auctions_mtx, NULL);
    pthread_mutex_init(&persons_mtx, NULL);
}

void TumblingWindowJoin::streamProcess(int channel) {

    // global aggregation operator
    if (rank == 0) {
        
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
}

void TumblingWindowJoin::updateInputMessages(list<Message*>*const tmpMessages, const int channel){
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

void TumblingWindowJoin::processInputMessage(Message*const inMessage) {

    // message = [events][message type][message id] <- buffer end
    int message_id = Serialization::unwrap<int>(inMessage);
    int timestamp = Serialization::unwrap<int>(inMessage);
    JoinInputType message_type = Serialization::unwrap<JoinInputType>(inMessage);
    // cout << "[join] received msgid : " << message_id << " with message type : " << message_type << " (thread " << pthread_self() << ")" << endl;

    // process input message depending on content type
    // basically, store them until window is full
    const int window_id = message_id / (window_duration * PER_SEC_MSG_COUNT * worldSize);

    switch (message_type)
    {
    case JoinInputType::AuctionSelectionType:
        {
            processAuctionMessage(inMessage, window_id);
            break;
        }

    case JoinInputType::PersonSelectionType:
        {
            processPersonMessage(inMessage, window_id);
            break;
        }

    default:
        {
            throw "[Tumbling window join](process input message) Message type is not recognized.";
        }
    }

    // update window completeness & end time
    pthread_mutex_lock(&mtx);
    windows[window_id].completeness++;
    windows[window_id].timestamp_end = std::max(windows[window_id].timestamp_end, timestamp);

    // when first window is full (keeps the order), compute resulting aggregation
    if (isCurrentWindowComplete()) {
        computeAggregates();
        Message* outMessage = buildOutputMessage();
        send(outMessage);
    }
    pthread_mutex_unlock(&mtx);

}

void TumblingWindowJoin::computeAggregates(){
    WindowInformation& current_window = windows.begin()->second;

    for (auto & auction : current_window.auctions)
    {
        auto pperson = current_window.persons.find(auction.seller_id);
        if (pperson != current_window.persons.end()) {
            current_window.aggregates.push_back(combine(auction, pperson->second));
        }
    }

}

bool TumblingWindowJoin::isCurrentWindowComplete(){

    try
    {
        WindowInformation& current_window = windows.begin()->second;
        if (current_window.completeness == PER_SEC_MSG_COUNT * worldSize * 2 * window_duration){
            return true; // window is complete
        } else {
            return false; // window is no complete, wait for a few more messages
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "[TumblingWindowJoin](isCurrentWindowComplete) No window initialized" << '\n';
    }    
}

void TumblingWindowJoin::processAuctionMessage(Message*const inMessage, const int window_id){
    // message header has been removed, so here we process the message content
    // when the message contains auction events, we simply process each auction
    // which will update the data structures as needed
    size_t nof_events = inMessage->size / sizeof(AuctionSelection);

    for (size_t i = 0 ; i < nof_events ; i++){
        AuctionSelection auction = Serialization::unwrap<AuctionSelection>(inMessage);
        processAuctionEvent(auction, window_id); // process values
    }
}

void TumblingWindowJoin::processPersonMessage(Message*const inMessage, const int window_id){
    // message header has been removed, so here we process the message content
    // when the message contains Person events, we simply process each person
    // which will update the data structures as needed
    size_t nof_events = inMessage->size / sizeof(PersonSelection);

    for (size_t i = 0 ; i < nof_events ; i++){
        PersonSelection person = Serialization::unwrap<PersonSelection>(inMessage);
        processPersonEvent(person, window_id); // process values
    }
}

void TumblingWindowJoin::processAuctionEvent(AuctionSelection& auction, const int window_id) {
    pthread_mutex_lock(&auctions_mtx);
    windows[window_id].auctions.push_back(auction);
    pthread_mutex_unlock(&auctions_mtx);
}

void TumblingWindowJoin::processPersonEvent(PersonSelection& person, const int window_id) {
    // when we get a new person, we can simply store its information
    pthread_mutex_lock(&persons_mtx);
    windows[window_id].persons.emplace(person.id, person);
    pthread_mutex_unlock(&persons_mtx);
}

Message*const TumblingWindowJoin::buildOutputMessage(){
    // read aggregation vector and add content to a message buffer

    // message = [auction-person combinations][message id] <- buffer end
    auto it = windows.begin();
    const int window_id = it->first;
    WindowInformation& current_window = it->second;
    size_t message_capacity = current_window.aggregates.size() * sizeof(JoinOutput);

    // if an operator is built on this one, overriding initMessage will allow you
    // to increase or decrease the size of your output message(s) at will, that's
    // why we don't use the new keyword.
    Message*const message = initMessage(message_capacity);

    for (JoinOutput combination : current_window.aggregates){
        Serialization::wrap<JoinOutput>(combination, message);
    }

    Serialization::wrap<int>(current_window.timestamp_end, message);
    Serialization::wrap<int>(window_id, message);

    // remove first window
    windows.erase(it);

    return message;
}

Message*const TumblingWindowJoin::initMessage(size_t message_capacity){
    return new Message(message_capacity + 2*sizeof(int));
}

JoinOutput TumblingWindowJoin::combine(AuctionSelection& auction, PersonSelection& person){
    JoinOutput combination;
    combination.seller_id = auction.seller_id;
    combination.reserve_price = auction.reserve_price;
    combination.auction_id = auction.id;
    strcpy(combination.seller_name, person.name);

    // no need to copy the name string because the person data
    // is kept by the manager until the window output is sent
    // so at least until this combination is wrapped to a
    // message buffer ; though I get an invalid array assignment
    // error if I don't use strcpy

    return combination;
}


void TumblingWindowJoin::send(Message*const message){

    // send message to the only collector that should run
    // so there is no need to create copies of the message

    size_t nof_operators_next = next.size();

    if (nof_operators_next != 1) {
        throw "[Tumbling window join](send) Incorrect number of collectors.";
    } else {
        send(message, 0);
    }
}

void TumblingWindowJoin::send(Message*const message, const int channel){
    pthread_mutex_lock(&senderMutexes[channel]);
    outMessages[channel].push_back(message);
    pthread_cond_signal(&senderCondVars[channel]);
    pthread_mutex_unlock(&senderMutexes[channel]);
}
