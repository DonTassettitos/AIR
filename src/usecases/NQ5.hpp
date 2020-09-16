#pragma once
#include "../dataflow/Dataflow.hpp"

namespace nexmark {

/**
 * SORTING & MONITORING NEW WIDS
 */
class NQ5: public Dataflow {

public:
	Vertex *generator, *filter, *splitter, *locagg, *collector;

	// processing-time aggregation parameters
	static const int window_duration = 10; // 3600
	static const int window_shift = 2; // 60

	NQ5(unsigned long tp);

	~NQ5();

};
};