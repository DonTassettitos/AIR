#include "SCB.hpp"
#include "../scb/EventGenerator.hpp"
#include "../scb/Aggregator.hpp"
#include "../scb/EventCollector.hpp"

using namespace scb;

SCB::SCB() : Dataflow() {

	generator = new scb::EventGenerator(1, rank, worldSize);
	aggregator_count = new scb::Aggregator(2, rank, worldSize);
	collector = new scb::EventCollector(3, rank, worldSize);

	addLink(generator, aggregator_count);
	addLink(aggregator_count, collector);

	generator->initialize();
	aggregator_count->initialize();
	collector->initialize();
}

SCB::~SCB() {

	delete generator;
	delete aggregator_count;
	delete collector;
}