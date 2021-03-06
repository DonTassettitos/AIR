#include "../usecases/SCBFW.hpp"
#include "Aggregator.hpp"

using namespace scb_fw;

/**
 * Simple constructor to initialize the values of the count aggregator.
 * */
Aggregator::Aggregator(const int tag, const int rank, const int worldSize) :
FlowWrappingAggregator<Event>(tag, rank, worldSize){
    setBaseline('w');
}

/**
 * We modify the stream processing protocol because we only want one instance of the aggregator.
 * */
void Aggregator::streamProcess(int channel){

    if (rank == SCBFW::aggregator_rank){
        FlowWrappingAggregator<Event>::streamProcess(channel);
    }
}

/**
 * Implementation of a tumbling window with the duration defined in the usecase definition file.
 * (usecases/SCBFW.hpp)
 * 
 * Usually we would have a timestamp on each and every event that would help us to compute the 
 * corresponding window id(s) (plural if we were using a sliding window for example). But here
 * ther generator directly sends us the window ids to make it easier
 * */
vector<int> Aggregator::getWindowIDs(const Event& event){
    return vector<int>({event.first}); // left member of the pair is the window_id in that case
}

/**
 * Calculation of max completeness.
 * 
 * Each generator instance (worldSize) is generating PER_SEC_MSG_COUNT mesages per second
 * (window_duration seconds). And they may have been split between previous.size() operators.
 * */
int Aggregator::getMaxCompleteness(){
    return SCBFW::window_duration * PER_SEC_MSG_COUNT * previous.size() * worldSize;
}
