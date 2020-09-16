#include "SCBWS.hpp"
#include "../scb_ws/EventGenerator.hpp"
#include "../scb_ws/Aggregator.hpp"
#include "../scb_ws/EventCollector.hpp"

using namespace scb_ws;

SCBWS::SCBWS() : Dataflow() {

	generator = new EventGenerator(1, rank, worldSize);
	aggregator_sort = new Aggregator(2, rank, worldSize);
	collector = new EventCollector(3, rank, worldSize);

	addLink(generator, aggregator_sort);
	addLink(aggregator_sort, collector);

	generator->initialize();
	aggregator_sort->initialize();
	collector->initialize();
}

SCBWS::~SCBWS() {

	delete generator;
	delete aggregator_sort;
	delete collector;
}