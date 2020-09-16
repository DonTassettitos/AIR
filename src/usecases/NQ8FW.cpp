#include "NQ8FW.hpp"

#include "../nexmark_new_sellers_fw/EventGenerator.hpp"
#include "../nexmark_new_sellers_fw/PersonsFilter.hpp"
#include "../nexmark_new_sellers_fw/AuctionsFilter.hpp"
#include "../nexmark_new_sellers_fw/PersonAttributesSelector.hpp"
#include "../nexmark_new_sellers_fw/AuctionAttributesSelector.hpp"
#include "../nexmark_new_sellers_fw/TumblingWindowJoin.hpp"
#include "../nexmark_new_sellers_fw/EventCollector.hpp"

using nexmark::NQ8FW;
using nexmark_new_sellers_fw::EventGenerator;
using nexmark_new_sellers_fw::PersonsFilter;
using nexmark_new_sellers_fw::AuctionsFilter;
using nexmark_new_sellers_fw::PersonAttributesSelector;
using nexmark_new_sellers_fw::AuctionAttributesSelector;
using nexmark_new_sellers_fw::TumblingWindowJoin;
using nexmark_new_sellers_fw::EventCollector;

NQ8FW::NQ8FW(unsigned long throughput) :
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

NQ8FW::~NQ8FW() {

	delete generator;
	delete pfilter;
	delete afilter;
	delete pselector;
	delete aselector;
	delete join;
	delete collector;
}

