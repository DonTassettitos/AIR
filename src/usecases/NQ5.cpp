#include "NQ5.hpp"

#include "../nexmark_gen/EventGenerator.hpp"
#include "../nexmark_hot_items/EventCollector.hpp"
#include "../nexmark_hot_items/BidFilter.hpp"
#include "../nexmark_hot_items/EventSharder.hpp"
#include "../nexmark_hot_items/GroupbyAuction.hpp"

using nexmark::NQ5;
using nexmark_gen::EventGenerator;
using nexmark_hot_items::BidFilter;
using nexmark_hot_items::EventCollector;
using nexmark_hot_items::EventSharder;
using nexmark_hot_items::GroupbyAuction;

NQ5::NQ5(unsigned long throughput) :
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

NQ5::~NQ5() {

	delete generator;
	delete filter;
	delete splitter;
	delete locagg;
	delete collector;
}

