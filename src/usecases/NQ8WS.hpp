#pragma once

#include "../dataflow/Dataflow.hpp"

using namespace std;

namespace nexmark {

/**
 * SORTING & PUNCTUATION
 * 
 * Implementation of NEXMark benchmark's query 8
 * with automatic sorting of the incoming events
 * (only for the join operation now, as the other
 * operations are non-blocking)
 * 
 * It ensures envents are sorted, and emulates
 * punctuation of NAIAD system by checking when
 * each instance finish to process the current
 * window
 */
class NQ8WS: public Dataflow {

public:

	static const int window_duration = 2; // seconds

	Vertex *generator, *pfilter, *afilter, *pselector, *aselector, *join, *collector;

	NQ8WS(unsigned long tp);

	~NQ8WS();

};
};
