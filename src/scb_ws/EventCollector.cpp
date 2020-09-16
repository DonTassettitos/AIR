#include "EventCollector.hpp"

using namespace scb_ws;

/**
 * Simple constructor.
 * */
EventCollector::EventCollector(const int tag, const int rank, const int worldSize) :
scb::EventCollector(tag, rank, worldSize) {
	setBaseline('s');
}