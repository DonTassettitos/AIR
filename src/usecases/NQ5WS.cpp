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
 * NQ5WS.cpp
 *
 *  Created on: April 2, 2020
 *      Author: damien.tassetti
 */

#include "NQ5WS.hpp"

#include "../nexmark_gen/EventGenerator.hpp"
#include "../nexmark_hot_items_ws/EventCollector.hpp"
#include "../nexmark_hot_items/BidFilter.hpp"
#include "../nexmark_hot_items/EventSharder.hpp"
#include "../nexmark_hot_items_ws/GroupbyAuction.hpp"

using nexmark::NQ5WS;
using nexmark_gen::EventGenerator;
using nexmark_hot_items::BidFilter;
using nexmark_hot_items_ws::EventCollector;
using nexmark_hot_items::EventSharder;
using nexmark_hot_items_ws::GroupbyAuction;

NQ5WS::NQ5WS(unsigned long throughput) :
		Dataflow() {

	generator = new EventGenerator(1, rank, worldSize, throughput);
	filter = new BidFilter(2, rank, worldSize);
	splitter = new EventSharder(3, rank, worldSize);
	locagg = new GroupbyAuction(4, rank, worldSize);
	collector = new EventCollector(5, rank, worldSize);

	addLink(generator, filter);
	addLink(filter, splitter);
	addLink(splitter, locagg);
	addLink(locagg, collector);

	generator->initialize();
	filter->initialize();
	splitter->initialize();
	locagg->initialize();
	collector->initialize();
}

NQ5WS::~NQ5WS() {

	delete generator;
	delete filter;
	delete splitter;
	delete locagg;
	delete collector;
}

