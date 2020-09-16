#include "EventGenerator.hpp"
#include "../usecases/SCBWS.hpp"

using namespace scb_ws;

EventGenerator::EventGenerator(const int tag, const int rank, const int worldSize) :
scb::EventGenerator(tag, rank, worldSize)
{
	this->window_duration = scb_ws::SCBWS::window_duration;
	this->delay_probability = scb_ws::SCBWS::delay_probability;
	this->delay_min_duration = scb_ws::SCBWS::delay_min_duration;
	this->delay_max_duration = scb_ws::SCBWS::delay_max_duration;
	this->aggregator_rank = scb_ws::SCBWS::aggregator_rank;

	setBaseline('s'); // sort baseline
}