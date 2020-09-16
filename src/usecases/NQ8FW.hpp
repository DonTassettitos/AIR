#pragma once

#include "../dataflow/Dataflow.hpp"

using namespace std;

namespace nexmark {

/**
 * COUNT (FLOW-WRAPPING)
 * 
 * Implementation of NEXMark benchmark's query 8
 * using wrapping units on top of the messages, instead
 * of an internal implementation of the message counting
 */
class NQ8FW: public Dataflow {

public:

	static const int window_duration = 2; // seconds

	Vertex *generator, *pfilter, *afilter, *pselector, *aselector, *join, *collector;

	NQ8FW(unsigned long tp);

	~NQ8FW();

};
};
