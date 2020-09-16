#ifndef COMMUNICATION_WINDOW_HPP_
#define COMMUNICATION_WINDOW_HPP_

#include <iostream>

// temporary fix:
// might cause bad_alloc if too big, and segfault if too small
static const size_t DEFAULT_WINDOW_SIZE = MAX_MSG_SIZE;
static const int PER_SEC_MSG_COUNT = NOF_MESSAGES_PER_SEC;

// Everything below this line won't be modified
static const long THROUGHPUT = 25000000;
static const int EVENT_SIZE = 136;
static const int MAX_WRAPPER_SIZE = 1;
static const long AGG_WIND_SPAN = 10000;

static const long WINDOW_SIZE = (THROUGHPUT / PER_SEC_MSG_COUNT) * EVENT_SIZE
		+ MAX_WRAPPER_SIZE + 4;

class Window {
public:
	char* buffer;
	int size;
	int capacity;
	Window();
	Window(int capacity);
	virtual ~Window();
	virtual void clear();
	virtual void resize(int capacity);
};

#endif /* COMMUNICATION_WINDOW_HPP_ */
