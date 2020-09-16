#include "NQ8WS.hpp"

#include "../nexmark_new_sellers/EventGenerator.hpp"
#include "../nexmark_new_sellers/PersonsFilter.hpp"
#include "../nexmark_new_sellers/AuctionsFilter.hpp"
#include "../nexmark_new_sellers/PersonAttributesSelector.hpp"
#include "../nexmark_new_sellers/AuctionAttributesSelector.hpp"
#include "../nexmark_new_sellers_ws/TumblingWindowJoin.hpp"
#include "../nexmark_new_sellers_ws/EventCollector.hpp"

using nexmark::NQ8WS;
using nexmark_new_sellers::EventGenerator;
using nexmark_new_sellers::PersonsFilter;
using nexmark_new_sellers::AuctionsFilter;
using nexmark_new_sellers::PersonAttributesSelector;
using nexmark_new_sellers::AuctionAttributesSelector;
using nexmark_new_sellers_ws::TumblingWindowJoin;
using nexmark_new_sellers_ws::EventCollector;

NQ8WS::NQ8WS(unsigned long throughput) :
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

NQ8WS::~NQ8WS() {

	delete generator;
	delete pfilter;
	delete afilter;
	delete pselector;
	delete aselector;
	delete join;
	delete collector;
}

