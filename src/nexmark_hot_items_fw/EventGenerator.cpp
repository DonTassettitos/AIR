#include "EventGenerator.hpp"
#include "../usecases/NQ5FW.hpp"

#include <mpi.h>

using namespace nexmark_hot_items_fw;

EventGenerator::EventGenerator(int tag, int rank, int worldSize, unsigned long tp) :
OriginalEventGenerator(tag, rank, worldSize, tp) {
}

Message* EventGenerator::initMessage(size_t capacity) {
    return OriginalEventGenerator::initMessage(capacity + sizeof(WrapperUnit));
}

void EventGenerator::getNextMessage(Message* message, const int events_per_msg) {
    OriginalEventGenerator::getNextMessage(message, events_per_msg);

    WrapperUnit wu = {}; // the values don't really matter here

    Serialization::wrap<WrapperUnit>(wu, message);

    // after that, message_if is automatically appened
}
