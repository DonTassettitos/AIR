#pragma once
#include "../dataflow/BasicVertex.hpp"
#include "../communication/Message.hpp"
#include "../serialization/Serialization.hpp"

#include <mpi.h>
#include <deque>

namespace scb { // add class to benchmark namespace

using WindowID = int;
using Event = pair<WindowID, int>; // we use a window id instead of a timestamp for easier implementation

/**
 * Random generator of integers (100 values + baseline + msgid).
 * This implementation is dedicated to the count baseline.
 * 
 * Uses the default char typename, but anyway we won't read events
 * so the input's typename could be anything really, and it wouldn't
 * really matter.
 * */
class EventGenerator: public BasicVertex<> {

public:

	EventGenerator(const int tag, const int rank, const int worldSize);

	void streamProcess(const int channel);

protected:

	virtual vector<output_data> generateMessages(const unsigned int quantity);
	virtual Event generateEvent(WindowID id);
	virtual void wait(double call_time, double max_duration=(double)1);
	virtual void send(vector<output_data> messages);
	virtual vector<output_data> delay(vector<output_data> messages);

	// generators only have one thread per rank so we can store some values here
	int iteration = 0; 
	int starting_second = (int)MPI_Wtime();
	unsigned int window_duration = 2;
	static const long int precision_factor = 1000000;
	int delay_probability = 10; // 10%
	int delay_min_duration = 1; // 1s
	int delay_max_duration = 4;
	int aggregator_rank = 0;

	deque<vector<output_data>> messages_with_delays;
};
};