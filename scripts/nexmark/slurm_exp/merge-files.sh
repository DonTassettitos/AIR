#!/bin/bash

# description : merges all results together (when the number of nodes is the same)
# usage : ./merge-files

# possible improvements:
# - add options to change which directory we are looking through
# - change output file name

if [ ! $# -eq 1 ]; then
    echo "Usage: merge-files <directory>"
    exit 0
fi

DIRECTORY=$1
NOF_NODES_MAX=4
NOF_DATAFLOW_MIN=4
NOF_DATAFLOW_MAX=28
THROUGHPUT_MAX=1000000 # 1M

loop_nodes(){
    NOF_NODES=1
    while [ $NOF_NODES -le $NOF_NODES_MAX ]
    do
        rm -f ${DIRECTORY}/results-${NOF_NODES}.csv

        loop_query

        NOF_NODES=$(( $NOF_NODES * 2 ))
    done
}

loop_query(){
    for QUERY in NQ5 NQ5FW NQ5WS NQ8 NQ8FW NQ8WS
    do
        loop_dataflows    
    done
}

loop_dataflows(){

    for NOF_DATAFLOWS in $( seq $NOF_DATAFLOW_MIN 4 $NOF_DATAFLOW_MAX )
    do
        loop_throughputs
    done
}

loop_throughputs(){ 
    THROUGHPUT=1000
    while [ $THROUGHPUT -le $THROUGHPUT_MAX ]
    do
        merge_files

        THROUGHPUT=$(( $THROUGHPUT * 10 ))
    done
}

merge_files(){

    FILENAME="${DIRECTORY}/${QUERY}-${NOF_NODES}-${NOF_DATAFLOWS}-${THROUGHPUT}.csv"
    if [ -f $FILENAME ]; then
        echo $FILENAME
        tail -n +4 "$FILENAME" > "$FILENAME.tmp" && mv "$FILENAME.tmp" "$FILENAME"
        sed -e "s/$/, ${THROUGHPUT}/" -i $FILENAME # add throughput column
        cat $FILENAME >> ${DIRECTORY}/results-${NOF_NODES}.csv
    fi

}

clean(){
    # removes files left from previous uses
    NOF_NODES=1
    while [ $NOF_NODES -le $NOF_NODES_MAX ]
    do
        FILENAME="${DIRECTORY}/results-${NOF_NODES}.csv"
        if [ -f $FILENAME ]; then
            rm $FILENAME
        fi

        NOF_NODES=$(( $NOF_NODES * 2 ))
    done
}

# Start
clean
loop_nodes
