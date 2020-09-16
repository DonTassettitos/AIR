#include "EventGenerator.hpp"
#include "../nexmark_gen/Random.hpp"
#include "../usecases/SCB.hpp"

#include <unistd.h> // usleep
#include <set> //test

using namespace scb;
using nexmark_gen::Random;

/**
 * Simple constructor to initialize the basic vertex values.
 * 
 * Throughput doesn't really matter because we are only trying
 * to debug the different baselines for now.
 * */
EventGenerator::EventGenerator(const int tag, const int rank, const int worldSize) :
BasicVertex<>(tag, rank, worldSize),
window_duration(scb::SCB::window_duration),
delay_probability(scb::SCB::delay_probability),
delay_min_duration(scb::SCB::delay_min_duration),
delay_max_duration(scb::SCB::delay_max_duration),
aggregator_rank(scb::SCB::aggregator_rank){
	setBaseline('c'); // we are implementing a generator for the count baseline
}

/**
 * We override the streamProcess method because we don't
 * need to read input messages. We only need to wait a 
 * certain amount of time before sending messages.
 * */
void EventGenerator::streamProcess(const int channel){
	
	while(ALIVE){
		// removed fetchNextMessage call

		// input message content doesn't matter as we won't even look at it
		// so we define another method that generates X random messages
		vector<output_data> out = generateMessages(PER_SEC_MSG_COUNT);

		// from time to time, messages are delayed
		// so here we remove the messages that are delayed
		vector<output_data> out2 = delay(move(out));

		// once the messages are generated we wait until the end of the second
		// then send the previously built messages.
		wait(MPI_Wtime());
		send(move(out2));
	}
}

/**
 * New method to generate random messages.
 * 
 * - quantity is the number of messages we want to generate.
 * */
vector<output_data> EventGenerator::generateMessages(const unsigned int quantity){

	vector<output_data> res; res.reserve(quantity);

	unsigned int message_id;
	unsigned int window_id;
	for (unsigned int i = 0 ; i < quantity; i++){
		// preparation
		message_id = (i + iteration * quantity) * worldSize + rank;
		window_id = iteration / window_duration;

		message_ptr message = createMessage(100 * sizeof(Event));

		// fill message buffer
		for(size_t j = 0 ; j<100 ; j++){
			Serialization::wrap<Event>(generateEvent(window_id), message.get());
		}

		// add message id and baseline
		Serialization::wrap<char>(getBaseline(), message.get());
		Serialization::wrap<int>(message_id, message.get());

		// add message to output
		destination dest = vector<int>({aggregator_rank});
		res.push_back(make_pair(move(message), dest));
	}

	return res;
}

/**
 * Generate a new event. Here it is just a random integer.
 * 
 * The random generator is taken from nexmarkgen/Random.hpp
 * */
Event EventGenerator::generateEvent(WindowID id){
	return make_pair(id, Random::getRandomInt(0, 100));
}

/**
 * Randomly delays messages. 
 * */
vector<output_data> EventGenerator::delay(vector<output_data> input){
	vector<output_data> out;
	vector<output_data> tmp;

	// update stored messages remaining delay
	vector<output_data> messages_with_finished_delay;
	if (!messages_with_delays.empty()){
		messages_with_finished_delay = move(messages_with_delays.front());
		messages_with_delays.pop_front();
	}

	// check probability of delaying a message
	vector<size_t> indexes_to_keep;
	vector<size_t> indexes_to_delay;
	for (size_t i = 0; i < input.size(); i++){

		const int lotery = Random::getRandomInt(1, 100);
		if (lotery > this->delay_probability){
			indexes_to_keep.push_back(i); 
		} else {
			indexes_to_delay.push_back(i);
		}
	}

	// insert messages in the queue depending on how long they are delayed
	for (size_t& i : indexes_to_delay){
		const int delay = Random::getRandomInt(delay_min_duration, delay_max_duration);

		if (delay > messages_with_delays.size()){
			const int diff = delay - messages_with_delays.size();
			for (size_t j = 0; j < diff; j++){
				messages_with_delays.push_back(vector<output_data>()); // default value for delayed messages.
			}
		}

		messages_with_delays[delay-1].push_back(move(input[i]));
	}

	// debug: observe messages sent normally, messages that finished being delayed and messages that are still kept because they are delayed

	// size_t sum = 0;
	// for (auto const& r : messages_with_delays){
	// 	sum += r.size();
	// }
	// cout << indexes_to_keep.size() << " + " << messages_with_finished_delay.size() << "(" << sum << ")" << endl;

	// out = concatenation of these messages
	// => question : is it necessary ?
	// (i know it is to test flow wrapping with multiple wrapping
	// units but i'm not sure it's a situation we may ever encounter
	// because delayed messages would never merge with other messages
	// normally (because they should already be sent))

	// for now let's send the messages normally (without concatenation)
	// to see if their content is still readable (done, you can try
	// by outputing tmp instead of out)

	for (auto const& i : indexes_to_keep){
		tmp.push_back(move(input[i]));
	}

	for (size_t i = 0; i < messages_with_finished_delay.size(); i++){
		tmp.push_back(move(messages_with_finished_delay[i]));
	}

	// note : output data values of the input vector are destroyed at the end of the scope
	return tmp;
}

/**
 * Wait for a certain time duration to be finished. The actual
 * waiting time could be much less, depending on how long the 
 * message generation lasts.
 * 
 * Time is in usec because we can control the level of precision
 * that way.
 * */
void EventGenerator::wait(double call_time, double max_duration){
	const long int call_time_p = call_time * EventGenerator::precision_factor;
	const long int max_duration_p = max_duration * EventGenerator::precision_factor;
	const long int expected_time_lower = (long int)(starting_second + iteration) * EventGenerator::precision_factor;
	const long int expected_time_upper = expected_time_lower + max_duration * EventGenerator::precision_factor;

	if (call_time_p < expected_time_lower){
		cerr << "! Careful : event generator " << rank << " is too fast by " << expected_time_lower - call_time_p << " usec\n";
	} else if (call_time_p < expected_time_upper){
		while (MPI_Wtime() * EventGenerator::precision_factor < expected_time_upper ){
			usleep(100);
		}
	} else {
		cerr << "! Careful : event generator " << rank << " is too slow by " << call_time_p - expected_time_upper << " usec\n";
	}
}

/**
 * Decorate the send method to update the iteration value
 * after all the current iteration's messages are sent
 * */
void EventGenerator::send(vector<output_data> messages){
	BasicVertex<>::send(move(messages));
	iteration++;
}
