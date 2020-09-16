#include "EventGenerator.hpp"
#include "../usecases/SCBFW.hpp"

using namespace scb_fw;

EventGenerator::EventGenerator(const int tag, const int rank, const int worldSize) :
scb::EventGenerator(tag, rank, worldSize)
{
	this->window_duration = scb_fw::SCBFW::window_duration;
	this->delay_probability = scb_fw::SCBFW::delay_probability;
	this->delay_min_duration = scb_fw::SCBFW::delay_min_duration;
	this->delay_max_duration = scb_fw::SCBFW::delay_max_duration;
	this->aggregator_rank = scb_fw::SCBFW::aggregator_rank;

	setBaseline('w'); // flow-wrapping baseline
    increaseHeaderSize(sizeof(WrapperUnit));
}

/**
 * Same generation method as scb (count) but this time we add a wrapper unit.
 * */
vector<output_data> EventGenerator::generateMessages(const unsigned int quantity){
    vector<output_data> out = move(scb::EventGenerator::generateMessages(quantity));

    WrapperUnit wu;
    wu.window_start_time = iteration / window_duration; // window_id 
    wu.completeness_tag_numerator = 1;
    wu.completeness_tag_denominator = SCBFW::window_duration * PER_SEC_MSG_COUNT * worldSize; // should be the same in the aggregator

    for(size_t i = 0; i < out.size(); i++){
        Serialization::wrap<WrapperUnit>(wu, out[i].first.get());
    }

	return out;
}

/**
 * Same implementation as the count baseline
 * But we concatenate messages at the end
 * */
vector<output_data> EventGenerator::delay(vector<output_data> input){

	vector<output_data> out;
	vector<output_data> tmp = move(scb::EventGenerator::delay(move(input)));

	// split in PER_SEC_MSG_COUNT messages
	vector<vector<output_data>> splitter(PER_SEC_MSG_COUNT);

	for(size_t i = 0; i < tmp.size(); i++){
		splitter[i % PER_SEC_MSG_COUNT].push_back(move(tmp[i]));
	}

	for(size_t i = 0; i < PER_SEC_MSG_COUNT; i++){
		if (!splitter[i].empty()) {
			output_data concat = concatenate(move(splitter[i])); // also add the number of wrapper units even if there is only one
			// cout << "nof wu: " << Serialization::read_back<int>(concat.first.get()) << endl;
			// cout << "window: " << Serialization::read_back<WrapperUnit>(concat.first.get(), sizeof(int)).window_start_time << endl;
			out.push_back(move(concat));
		}
	}
	return out;
}

/**
 * Concatenates a number of messages in order to maintain the
 * same number of messages per second.
 * 
 * Note : what happens when the messages have different destinations ????
 * => for now we assume they have the same destination every time
 * */
output_data EventGenerator::concatenate(vector<output_data> input){

	if (input.empty()){
		throw "~SCBFW~[EventGenerator](concatenate) Input vector should have at least one message, otherwise we have no message id to give.";
	}
	
	// compute sum of content size
	// buffer size without the header size to get only the size of the content
	size_t size = 0;
	for (size_t i = 0; i < input.size(); i++){
		size += input[i].first->size - getHeaderSize();
	}

	// create new output data
	message_ptr out = createMessage(size + sizeof(WrapperUnit) * (input.size() - 1) + sizeof(int)); // 1 wrapper unit per message concatenated and A int to indicate how many wrapper units there are
	destination dest = vector<int>({0}); // copy vector, cf. note above method

	// fill content (copy only the content of each message and not the header)
	vector<WrapperUnit> units;
	for (size_t i = 0; i < input.size(); i++){
		Serialization::append(out.get(), input[i].first.get(), getHeaderSize());
		WrapperUnit wu = Serialization::unwrap<WrapperUnit>(input[i].first.get());
		units.push_back(wu);
	}

	// and finally, add the header (baseline + msgid + wrapper units + nof wrapper units)
	int message_id = readMessageID(input[0].first); 
	char baseline = readBaseline(input[0].first);
	Serialization::wrap<char>(baseline, out.get());
	Serialization::wrap<int>(message_id, out.get());
	for(size_t i = 0; i < units.size(); i++){
		Serialization::wrap<WrapperUnit>(units[i], out.get());
	}
	Serialization::wrap<int>(units.size(), out.get());

	return make_pair(move(out), dest);
}