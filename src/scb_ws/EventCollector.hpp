#pragma once
#include "../scb/EventCollector.hpp"

namespace scb_ws {
class EventCollector: public scb::EventCollector {

public:

	EventCollector(const int tag, const int rank, const int worldSize);

};
};