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
 * AuctionsManager.cpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "AuctionsManager.hpp"

#include <iostream>

using nexmark_new_sellers::AuctionsManager;
using nexmark_new_sellers::AuctionSelection;
using std::vector;
using std::pair;
using std::make_pair;
using std::cout;
using std::endl;

AuctionsManager::AuctionsManager(const int world_size, const int msg_frequency) :
world_size(world_size),
msg_frequency(msg_frequency),
completeness_time(-1) {
    // msg frequency is the number of messages sent per second per rank (i.e. PER_SEC_MSG_COUNT in most cases)
}

// TODO: remove maybe
void AuctionsManager::setCompletenessTime(const int second){
    completeness_time = second;
}

int AuctionsManager::getCompletenessTime() {
    return completeness_time;
}

void AuctionsManager::updateCompleteness(const int message_id) {
    const int second = message_id / (msg_frequency * world_size);

    completeness.emplace(second, 0); // if the pair doesn't exist, init with 0
    completeness[second]++; // we received one message for this second

    const int previous_second = second - 1;
    if (completeness[second] == world_size * msg_frequency && completeness_time == previous_second){
        completeness_time++;
        completeness.erase(second); // maybe we should use a list instead of a hashmap
    }
}

void AuctionsManager::addToWaitingList(const AuctionSelection& auction, const int message_id) {
    const int second = message_id / (msg_frequency * world_size);
    waitingList[second].push_back(make_pair(auction, message_id));
}

vector<pair<AuctionSelection, int>>& AuctionsManager::getWaitingList(const int second){
    return waitingList[second];
}

void AuctionsManager::removeWaitingList(const int second){
    waitingList.erase(second);
}