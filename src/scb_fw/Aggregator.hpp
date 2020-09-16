#pragma once
#include "../dataflow/FlowWrappingAggregator.hpp"

using namespace std;

namespace scb_fw {

using WindowID = int;
using Event = pair<WindowID, int>;

class Aggregator: public FlowWrappingAggregator<Event> {

public:

	Aggregator(int tag, int rank, int worldSize);
	void streamProcess(int channel);

protected:
	
	vector<int> getWindowIDs(const Event& event);
	int getMaxCompleteness();

	// default window processing is counting events.

};
};