#pragma once
#include "../dataflow/BasicVertex.hpp"

namespace scb {
class EventCollector: public BasicVertex<size_t> {

public:

	EventCollector(const int tag, const int rank, const int worldSize);
	void streamProcess(int channel);

protected:

	vector<output_data> processMessage(message_ptr message);

};
};