#pragma once

#include "../nexmark_gen/EventGenerator.hpp"

namespace nexmark_hot_items_fw {

using OriginalEventGenerator = nexmark_gen::EventGenerator;

class EventGenerator : public OriginalEventGenerator {
    public:
	EventGenerator(int tag, int rank, int worldSize, unsigned long tp);

    // increase message capacity
    Message* initMessage(size_t capacity);

    // add wrapper unit to message
	void getNextMessage(Message* message, const int events_per_msg);
};

};