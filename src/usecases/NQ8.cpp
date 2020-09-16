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
 * NQ8.cpp
 *
 *  Created on: May 15, 2020
 *      Author: damien.tassetti
 */

#include "NQ8.hpp"

#include "../nexmark_new_sellers/EventGenerator.hpp"
#include "../nexmark_new_sellers/PersonsFilter.hpp"
#include "../nexmark_new_sellers/AuctionsFilter.hpp"
#include "../nexmark_new_sellers/PersonAttributesSelector.hpp"
#include "../nexmark_new_sellers/AuctionAttributesSelector.hpp"
#include "../nexmark_new_sellers/TumblingWindowJoin.hpp"
#include "../nexmark_new_sellers/EventCollector.hpp"

using nexmark::NQ8;
using nexmark_new_sellers::EventGenerator;
using nexmark_new_sellers::PersonsFilter;
using nexmark_new_sellers::AuctionsFilter;
using nexmark_new_sellers::PersonAttributesSelector;
using nexmark_new_sellers::AuctionAttributesSelector;
using nexmark_new_sellers::TumblingWindowJoin;
using nexmark_new_sellers::EventCollector;

NQ8::NQ8(unsigned long throughput) :
		Dataflow() {

	generator = new EventGenerator(1, rank, worldSize, throughput);
	pfilter = new PersonsFilter(2, rank, worldSize);
	afilter = new AuctionsFilter(3, rank, worldSize);
	pselector = new PersonAttributesSelector(4, rank, worldSize);
	aselector = new AuctionAttributesSelector(5, rank, worldSize);
	join = new TumblingWindowJoin(6, rank, worldSize, window_duration);
	collector = new EventCollector(7, rank, worldSize);

	addLink(generator, pfilter);
	addLink(generator, afilter);
	addLink(pfilter, pselector);
	addLink(afilter, aselector);
	addLink(pselector, join);
	addLink(aselector, join);
	addLink(join, collector);

	generator->initialize();
	pfilter->initialize();
	afilter->initialize();
	pselector->initialize();
	aselector->initialize();
	join->initialize();
	collector->initialize();
}

NQ8::~NQ8() {

	delete generator;
	delete pfilter;
	delete afilter;
	delete pselector;
	delete aselector;
	delete join;
	delete collector;
}

