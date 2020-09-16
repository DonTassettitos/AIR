#pragma once
#include "../dataflow/SortAggregator.hpp"

namespace scb_ws { // add class to benchmark namespace

using WindowID = int;
using Event = pair<WindowID, int>;

class Aggregator: public SortAggregator<Event> {

public:

	Aggregator(int tag, int rank, int worldSize);
	void streamProcess(int channel);

protected:

	vector<int> getWindowIDs(const Event& event);
	int getMaxCompleteness();

};
};