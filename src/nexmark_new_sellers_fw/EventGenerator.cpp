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
 * EventGenerator.cpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#include "EventGenerator.hpp"
#include "WrapperUnit.hpp"
#include "../usecases/NQ8FW.hpp"

using nexmark_new_sellers_fw::EventGenerator;
using nexmark_new_sellers_fw::OriginalEventGenerator;
// using nexmark_new_sellers_fw::WrapperUnit;

EventGenerator::EventGenerator(int tag, int rank, int worldSize, unsigned long tp):
OriginalEventGenerator(tag, rank, worldSize, tp){}

Message* EventGenerator::initMessage(size_t capacity){
    // generated messages will have only one wrapper unit
    // because tumbling windows do not overlap
    return OriginalEventGenerator::initMessage(capacity + sizeof(WrapperUnit));
}

void EventGenerator::addMessageId(const int message_id, Message* message){
    // add original message id
    OriginalEventGenerator::addMessageId(message_id, message);

    // add wrapper unit after that
    const int window_duration = nexmark::NQ8FW::window_duration;
    const int worldSize = OriginalEventGenerator::worldSize;
    const int nof_messages_per_window = worldSize * window_duration * PER_SEC_MSG_COUNT;

    WrapperUnit wu;
    wu.window_id = message_id / nof_messages_per_window;
    wu.completeness_numerator = 1;
    wu.completeness_denumerator = nof_messages_per_window;

    Serialization::wrap<WrapperUnit>(wu, message);
}
