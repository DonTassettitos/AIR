#include "EventCollector.hpp"

using namespace scb_fw;

/**
 * Simple constructor.
 * */
EventCollector::EventCollector(const int tag, const int rank, const int worldSize) :
scb::EventCollector(tag, rank, worldSize) {
	setBaseline('w');
}