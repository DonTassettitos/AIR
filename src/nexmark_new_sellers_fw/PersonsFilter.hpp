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
 * PersonsFilter.hpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#pragma once
#include "../nexmark_new_sellers/PersonsFilter.hpp"
#include "WrapperUnit.hpp"

namespace nexmark_new_sellers_fw {
using nexmark_gen::Person;
using OriginalPersonsFilter = nexmark_new_sellers::PersonsFilter;
using nexmark_new_sellers_fw::WrapperUnit;

/**
 * Number of wrappers : one
 * 
 * Blocking operator : no
 * 
 * Wrapper modification : none
 **/
class PersonsFilter: public OriginalPersonsFilter {

    public:
    PersonsFilter(int tag, int rank, int worldSize);

    protected:
    virtual void processInputMessage(Message*const inMessage);
    virtual void send(Message*const message, const int channel);
    virtual Message* initMessage(size_t capacity);

    private:
    WrapperUnit current_unit;
};
};