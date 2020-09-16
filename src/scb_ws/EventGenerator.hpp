#pragma once
#include "../scb/EventGenerator.hpp"

namespace scb_ws {

/**
 * Random generator of integers (100 values + baseline + msgid).
 * This implementation is dedicated to the count baseline.
 * 
 * Uses the default char typename, but anyway we won't read events
 * so the input's typename could be anything really, and it wouldn't
 * really matter.
 * */
class EventGenerator: public scb::EventGenerator {

public:

	// here we only need to make sure that the correct baseline is used.
	EventGenerator(const int tag, const int rank, const int worldSize);

};
};