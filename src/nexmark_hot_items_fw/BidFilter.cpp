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
 * BidFilter.cpp
 *
 *  Created on: April 16, 2020
 *      Author: damien.tassetti
 */

#include "BidFilter.hpp"
#include "../serialization/Serialization.hpp"

using namespace nexmark_hot_items_fw;

BidFilter::BidFilter(int tag, int rank, int worldSize) : OriginalBidFilter(tag, rank, worldSize) {
}

Message* BidFilter::initMessage(size_t capacity) {
    return OriginalBidFilter::initMessage(capacity + sizeof(WrapperUnit));
}

vector<Bid> BidFilter::filterBids(Message*const inMessage){
    this->wu = Serialization::unwrap<WrapperUnit>(inMessage);
    return OriginalBidFilter::filterBids(inMessage);
}

void BidFilter::getNextMessage(Message*& outMessage, vector<Bid> & bids) {
    // cout << bids[0].event_time << '\n';
    OriginalBidFilter::getNextMessage(outMessage, bids);
    Serialization::wrap<WrapperUnit>(wu, outMessage); // wrapper unit hasn't been modified
}