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
 * EventGenerator.hpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#pragma once

#include "../nexmark_gen/EventGenerator.hpp"

namespace nexmark_new_sellers {
using OriginalEventGenerator = nexmark_gen::EventGenerator;

/**
 * Vertex input : none (this is a generator)
 * Vertex output : Event (Person, Auction or Bid)(see PODTypes.hpp in src/nexmark_gen)
 * 
 * Parallel : no (there is only one thread per generator instance)
 * Distributed : yes (each generator instance generates part of the throughput)
 * 
 * It is a little different from the original version as it adds the time at which
 * the message must have been sent along the message id. This is done to calculate
 * the latency at the end of the dataflow.
 **/
class EventGenerator : public OriginalEventGenerator {
    public:
    EventGenerator(int tag, int rank, int worldSize, unsigned long tp);
    virtual Message* initMessage(size_t capacity) {return OriginalEventGenerator::initMessage(capacity + sizeof(int));};
    virtual void addMessageId(const int message_id, Message* message){Serialization::wrap(starting_second + iteration_count + 1, message); OriginalEventGenerator::addMessageId(message_id, message);};
};
};
