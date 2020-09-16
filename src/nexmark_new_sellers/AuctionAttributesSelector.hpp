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
 * AuctionAttributesSelector.hpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#pragma once
#include "../dataflow/Vertex.hpp"
#include "PODTypesExtension.hpp"

namespace nexmark_new_sellers {
using nexmark_new_sellers::AuctionSelection;

/**
 * Vertex input : Auction
 * Vertex output : AuctionSelection (Auction with reduced number of attributes to facilitate following manipulations)
 * 
 * Parallel : not implemented yet (we could shard Auction events between different selector instances, wich would activate multiple threads per operator and maybe accelerate the selection because there would be more workers on a non blocking operation)
 * Distributed : yes (each dataflow reduces its Auction events)
 **/
class AuctionAttributesSelector : public Vertex {
    public:
    AuctionAttributesSelector(int tag, int rank, int worldSize);
    void streamProcess(int channel);

    protected:
    virtual void updateInputMessages(list<Message*>*const tmpMessages, const int channel);
    virtual void processInputMessage(Message*const inMessage);
    virtual Message* initMessage(size_t capacity);
    virtual void getNextMessage(Message*& outMessage, vector<AuctionSelection> & bids);
    virtual vector<AuctionSelection> selectAttributes(Message*const inMessage);
    virtual void send(Message*const message);
    virtual void send(Message*const message, const int channel);
};
};