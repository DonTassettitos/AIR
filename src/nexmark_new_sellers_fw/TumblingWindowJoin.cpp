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
#include "WrapperUnit.hpp"
#include "../usecases/NQ8FW.hpp"

using nexmark_new_sellers_fw::TumblingWindowJoin;
using nexmark_new_sellers_fw::OriginalTumblingWindowJoin;
// using nexmark_new_sellers_fw::WrapperUnit;

TumblingWindowJoin::TumblingWindowJoin(int tag, int rank, int worldSize, int window_duration):
OriginalTumblingWindowJoin(tag, rank, worldSize, window_duration){
    window_duration = nexmark::NQ8FW::window_duration;

    pthread_mutex_init(&fw_mut, NULL);
};

void TumblingWindowJoin::processInputMessage(Message*const inMessage){

    WrapperUnit wu = Serialization::unwrap<WrapperUnit>(inMessage);

    pthread_mutex_lock(&fw_mut);
    completeness.emplace(wu.window_id, 0);
    completeness[wu.window_id] += wu.completeness_numerator; // if completeness_denominator is the same, but there is only one value for this dataflow

    OriginalTumblingWindowJoin::processInputMessage(inMessage);
}

void TumblingWindowJoin::send(Message*const message, const int channel){
    WrapperUnit wu;
    wu.window_id = completeness.begin()->first;
    wu.completeness_numerator = 1;
    wu.completeness_denumerator = 1; // window's events have been fully processed

    completeness.erase(completeness.begin());
    
    Serialization::wrap<WrapperUnit>(wu, message);

    OriginalTumblingWindowJoin::send(message, channel);
    pthread_mutex_unlock(&fw_mut);
}

/**
 * Overrides the previous detection of a complete window (that uses internally watermarks)
 * in favor of processing wrapper units.
 */
bool TumblingWindowJoin::isCurrentWindowComplete() {
    // multiplied by two, because we need that many messages from auctions thread and persons thread
    const int nof_messages_per_window = window_duration * worldSize * PER_SEC_MSG_COUNT * 2;

    // map is from smallest to highest, if that's not the case we should use completeness.end()
    if (completeness.begin()->second == nof_messages_per_window){
        return true;
    } else {
        pthread_mutex_unlock(&fw_mut);
        return false;
    }
}

Message*const TumblingWindowJoin::initMessage(size_t capacity){
    return OriginalTumblingWindowJoin::initMessage(capacity + sizeof(WrapperUnit));
}
