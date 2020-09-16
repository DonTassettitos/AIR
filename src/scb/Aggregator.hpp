#pragma once
#include "../dataflow/CountAggregator.hpp"

using namespace std;

namespace scb { // add class to benchmark namespace

using WindowID = int;
using Event = pair<WindowID, int>;

class Aggregator: public CountAggregator<Event> {

public:

	Aggregator(int tag, int rank, int worldSize);
	void streamProcess(int channel);

protected:
	
	vector<int> getWindowIDs(const Event& event);
	int getMaxCompleteness();
	// default window processing is counting events.

};
};