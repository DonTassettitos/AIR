#pragma once
#include "../dataflow/Dataflow.hpp"

using namespace std;

namespace scb_ws{
class SCBWS: public Dataflow {

public:

	Vertex *generator, *aggregator_sort, *collector;

	SCBWS();

	~SCBWS();

	static const unsigned int window_duration = 2;
	static const unsigned int aggregator_rank = 0;
	static const int delay_probability = 10; // %
	static const int delay_min_duration = 1; // s
	static const int delay_max_duration = 4; // s

};
};