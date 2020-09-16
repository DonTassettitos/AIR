#include "EventCollector.hpp"
#include "../usecases/SCB.hpp"

#include <mpi.h>

using namespace scb;

/**
 * Simple constructor.
 * */
EventCollector::EventCollector(const int tag, const int rank, const int worldSize) :
BasicVertex<size_t>(tag, rank, worldSize) {
	setBaseline('c');
}

/**
 * We replace the normal streamprocess method because we won't
 * send anything after this vertex.
 * 
 * Also, we only need one instance of this collector, just after
 * the aggregator.
 * */
void EventCollector::streamProcess(int channel) {
	if (rank == SCB::aggregator_rank) {

		if (channel == 0){
			cout << "window_id, count\n";
		}

		list<message_ptr> pthread_waiting_list;

		while (ALIVE) {
			message_ptr message = fetchNextMessage(channel, pthread_waiting_list);
			processMessage(move(message)); // no need to send a result
			// no message sent
		}

	}
}

vector<output_data> EventCollector::processMessage(message_ptr message){
	int window_id = readMessageID(message);
    char input_baseline = readBaseline(message);

    if(input_baseline != getBaseline()){
        throw "[EventCollector](processMessage) Unrecognized aggregation protocol.";
    }

    vector<size_t> events = readEvents(message);
	
	if (events.size() != 1){
		throw "[EventCollector](processMessage) Too many events in a single message.";
	}

	size_t count = events.front(); // message should contain one event whose value is the count of events.

	// debug
	cout << window_id << ", " << count << endl;
}
