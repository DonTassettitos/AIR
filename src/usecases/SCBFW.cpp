#include "SCBFW.hpp"
#include "../scb_fw/EventGenerator.hpp"
#include "../scb_fw/Aggregator.hpp"
#include "../scb_fw/EventCollector.hpp"

using namespace scb_fw;

SCBFW::SCBFW() : Dataflow() {

	generator = new EventGenerator(1, rank, worldSize);
	aggregator_wrapping = new Aggregator(2, rank, worldSize);
	collector = new EventCollector(3, rank, worldSize);

	addLink(generator, aggregator_wrapping);
	addLink(aggregator_wrapping, collector);

	generator->initialize();
	aggregator_wrapping->initialize();
	collector->initialize();
}

SCBFW::~SCBFW() {

	delete generator;
	delete aggregator_wrapping;
	delete collector;
}