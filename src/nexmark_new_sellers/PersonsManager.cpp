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
 * PersonsManager.cpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "PersonsManager.hpp"

#include <limits>
#include <iostream>

using std::numeric_limits;
using nexmark_new_sellers::PersonsManager;
using nexmark_new_sellers::PersonSelection;
using std::cout;
using std::endl;

PersonsManager::PersonsManager(const int world_size, const int msg_frequency) :
world_size(world_size),
msg_frequency(msg_frequency),
min_time_with_data(numeric_limits<int>::max()),
completeness_time(-1) {
}

int PersonsManager::getCompletenessTime() {
    return completeness_time;
}

void PersonsManager::updateCompleteness(const int message_id) {
    // move internal clock forward if we receive enough message for a given second
    int second = getSecond(message_id);
    // windows[second].emplace(, 0); // if it's the first time we receive a message this second, then init count with 0
    windows[second].completeness++; // then increase message count

    int previous_second = second - 1;
    while(completeness_time == previous_second && windows[second].completeness == world_size * msg_frequency) {
        // we received all the messages generated for the current second, so we can increase the completeness watermark
        // and remove the message count for the current second (after we finish processing all the auctions as well)
        completeness_time++;
        // windows.erase(second);

        // if we received all the messages of the next second before the current one's conter was filled
        // then we can directly increase the watermark again... until we get to a second for which we didn't
        // get all the messages yet.
        previous_second++;
        second++;
    }
}

bool PersonsManager::isUpToDateWith(const int message_id){
    // returns true if we received all the message for the current second
    // if we want something more precise and tell if we received all previous
    // messages when we didn't receive all the message for the current second, 
    // then we would need to store the exact message ids ; but for now we keep
    // it simple
    int second = getSecond(message_id);

    // completeness_time indicates if we received all the messages for the given
    // second. Although this is only for the messages containing PersonSelection
    // events. If this event manager is used to process AuctionSelection events
    // as well, then there will be two times as many events per second as there
    // should be.
    return completeness_time >= second;
}

bool PersonsManager::hasSeller(const int seller_id, const int window_id){
    return (windows[window_id].persons.count(seller_id) == 1);
}

const PersonSelection& PersonsManager::getSeller(const int seller_id, const int window_id){
    auto p = windows[window_id].persons.find(seller_id);
    if (p == windows[window_id].persons.end()){
        throw "[Persons manager](getSeller) Person id is not recognized.";
    } else {
        return p->second;
    }
}

void PersonsManager::store(PersonSelection& person, const int window_id){
    windows[window_id].persons[person.id] = person;

    // // window_to_maxid.emplace(window_id, 0);
    // if (person.id > windows[window_id].max_person_id){
    //     windows[window_id].max_person_id = person.id;
    // }
}

void PersonsManager::clearWindow(const int window_id){
    // removes all persons generated before the end of the current window
    // as they won't be useful for later processing

    // only works because personIndex is sorted by id
    windows.erase(window_id);
}


int PersonsManager::getSecond(const int message_id){
    return message_id / (world_size * msg_frequency);
}