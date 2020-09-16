#!/bin/bash

#===============================================================================
# Important :
#
# DO NOT RUN THIS SCRIPT ON THE FRONT END ! (use the `si` command before)
# Also, make sure to have your CMake configuration in the Release_iris folder.
# before using the script.
#
# Description :
# 
# This script will generate bash scripts usable on the Iris cluster of the 
# University of Luxembourg to run all the nexmark queries as a passive job.
#
# Documentation: https://hpc.uni.lu/users/docs/scheduler.html
#
# Author : Damien Tassetti, University of Luxembourg
# Usage : ./nexmark_slurm_exp.sh (cwd must be the script's directory)
#           -r
#               By default, the launchers will only be verified with --test-only
#               If this option is activated, the jobs are added to the queue
#               instead.
#
#           -c
#               Executable compilation is enabled by default. This disables it.
#
#           -t <nof tries>
#               Run all nodes, queries, baselines, ... as many times as
#               indicated. Default value is 1. 
#
# Recommanded usage : 
#           Use ./nexmark_slurm_exp.sh first to compile all the executables
#           without running the jobs for real.
#
#           Then use ./nexmark_slurm_exp.sh -c to make sure the executables are
#           readable from the current directory.
#
#           Then once you are certain of everything, run ./nexmark_slurm_exp.sh
#           -c -r to run the jobs for real this time.
#
#           It is not recommanded to use a high number of tries, because if the
#           number of jobs generated is too high (~400) then the hpc team might
#           kill the jobs and you will have to run the tests again.
#===============================================================================

# Configuration
NOF_TASKS_PER_NODE=4
NOF_CORES_PER_TASK=7
NOF_CORES_PER_NODE=28

OBSERVATION_TIME=00:05:00
EVENT_MAX_SIZE=352 # size of union type

# Script variables
RUN_SCRIPTS=0
COMPILE_EXECUTABLES=1
NOF_TRIES=1
VERBOSE=0

# This method shows how to use the script
show_usage() {
    echo "
    DO NOT RUN THIS SCRIPT ON THE FRONT END ! (use the `si` command before)
    
    Usage : ./nexmark_slurm_exp.sh (cwd must be the script's directory)

        -r
            By default, the launchers will only be verified with --test-only
            If this option is activated, the jobs are added to the queue
            instead.

        -c
            Disables the compilation of the AIR executables (enabled by default)
            Only use this option if you are certain all the necessary
            executables have already been compiled and in the correct directory.

            Depending on the throughput and the number of dataflows, the minimum
            default size of a message can change. So in order to get the best
            performance we can out of the engine, we must compile the 
            corresponding executables or find a fix to determine automatically
            what message size must be read when expecting a message.

        -t <nof tries>
            Run all nodes, queries, baselines, ... as many times as
            indicated. Default value is 1. 

        -v
            Run verbose script (shows the job scripts)
        "
    exit
}

# This method checks if we are given a number of nodes when calling the script.
args_check() {

    # check the option values
    local OPTIND OPTION
    while getopts "rhct:e:v" OPTION; do
        case "${OPTION}" in
            r)
                RUN_SCRIPTS=1
                ;;
            h)
                show_usage
                ;;
            c)
                COMPILE_EXECUTABLES=0
                ;;
            t)
                NOF_TRIES="${OPTARG}"
                ;;
            v)
                VERBOSE=1
                ;;
            *)
                show_usage
                ;;
        esac
    done
    shift $((OPTIND-1))
}

compile_executable() {
    local NOF_DATAFLOWS=$1
    local THROUGHPUT=$2

    local NOF_MESSAGES_PER_SEC=$(( $THROUGHPUT / $NOF_DATAFLOWS )) # nof events per dataflow
    NOF_MESSAGES_PER_SEC=$(( $NOF_MESSAGES_PER_SEC / 100000 )) # 100k events per message

    if [ $NOF_MESSAGES_PER_SEC -eq 0 ]; then
        NOF_MESSAGES_PER_SEC=1
    fi

    # calculate the max size of a message to compile the correct executable
    local MAX_MSG_SIZE=$(( 100 + $EVENT_MAX_SIZE * $THROUGHPUT ))
    local DENOMINATOR=$(( $NOF_MESSAGES_PER_SEC * $NOF_DATAFLOWS ))
    MAX_MSG_SIZE=$(( $MAX_MSG_SIZE / $DENOMINATOR )) # integer division
    MAX_MSG_SIZE=$(( 1 + $MAX_MSG_SIZE )) # ceil just in case

    local TEMPLATE_FILENAME=slurm_exp/window_template.hpp
    local WINDOW_FILE=$(m4 \
        -D MAX_MSG_SIZE=$MAX_MSG_SIZE \
        -D NOF_MESSAGES_PER_SEC=$NOF_MESSAGES_PER_SEC \
        $TEMPLATE_FILENAME)

    # remove output file if it already exists
    cd ../../src/communication
    if test -f Window.hpp; then
        rm Window.hpp &>/dev/null
    fi
    
    # write launcher
    echo "$WINDOW_FILE" >> Window.hpp
    cd -

    # compile executable
    # cd ../../Release/ => uncomment this line to test on local machine
    cd ../../Release_iris/
    make
    cd -

    mv ../../Release_iris/AIR ./slurm_exp/air_${NOF_DATAFLOWS}_${THROUGHPUT}.exe
}

run_nexmark_query() {
    local NOF_NODES=$1
    local NOF_DATAFLOWS=$2
    local THROUGHPUT=$3
    local QUERY=$4

    # compile executable if needed
    local AIR_EXECUTABLE_NAME=slurm_exp/air_${NOF_DATAFLOWS}_${THROUGHPUT}.exe

    if [ $COMPILE_EXECUTABLES -eq 1 ]; then
        # check if executable already exists
        if [ ! -f $AIR_EXECUTABLE_NAME ]; then
            compile_executable $NOF_DATAFLOWS $THROUGHPUT
        fi
    fi

    # setup execution parameters
    local JOB_NAME=$QUERY-N$NOF_NODES-D$NOF_DATAFLOWS-T$THROUGHPUT

    local LAUNCHER_TEMPLATE=slurm_exp/launcher_template.sh

    # default node_mem is in MB, add GB or T after if you want to change the memory
    local NODE_MEM=$(( 1 + $THROUGHPUT * 333 / 1000 / $NOF_NODES )) # experimental size needed based on 1000 e/s, 1 node and 28 df

    # keep to the max per node (128 GB)
    if [ $NODE_MEM -ge 110000 ]; then
        NODE_MEM=110000 # seems like 110 GB is the max, not 128 (else bad config...)
    fi

    local NOF_TASKS_PER_NODE=$(( $NOF_DATAFLOWS / $NOF_NODES )) # up to 28 tasks for 1 node, 14 for 2 nodes, 7 for 4 nodes
    local NOF_THREADS_PER_DATAFLOW=$(( 28 / $NOF_TASKS_PER_NODE ))
    local COMMAND=$(m4 \
    -D NOF_NODES="$NOF_NODES" \
    -D NOF_TASKS_PER_NODE="$NOF_TASKS_PER_NODE" \
    -D NOF_CORES_PER_TASK="$NOF_THREADS_PER_DATAFLOW" \
    -D NOF_DATAFLOWS="$NOF_DATAFLOWS" \
    -D OBSERVATION_TIME="$OBSERVATION_TIME" \
    -D JOB_NAME=$JOB_NAME \
    -D EXECUTABLE=$AIR_EXECUTABLE_NAME \
    -D THROUGHPUT=$THROUGHPUT \
    -D QUERY=$QUERY \
    -D NODE_MEM="${NODE_MEM}GB" \
    $LAUNCHER_TEMPLATE)

    if [ $VERBOSE -eq 1 ]; then
        echo "$COMMAND"
    fi

    # remove launcher if it already exists
    local COMMAND_FILENAME=slurm_exp/command.sh
    if test -f $COMMAND_FILENAME; then
        rm $COMMAND_FILENAME &>/dev/null
    fi
    
    # write launcher
    echo "$COMMAND" >> $COMMAND_FILENAME
    chmod +x $COMMAND_FILENAME

    # execute launcher depending on what execution mode has been given
    if [ $RUN_SCRIPTS -eq 1 ]; then
        # echo "running $COMMAND_FILENAME"
        sbatch $COMMAND_FILENAME
    else
        # echo "running (test) $COMMAND_FILENAME"
        sbatch --test-only $COMMAND_FILENAME
    fi

    rm $COMMAND_FILENAME
}

run_all_queries() {
    local NOF_NODES=$1
    local NOF_DATAFLOWS=$2
    local THROUGHPUT=$3

    for QUERY in NQ5 NQ5FW NQ5WS NQ8 NQ8FW NQ8WS; do
        if [ ! -f slurm_exp/observations/$QUERY-$NOF_NODES-$NOF_DATAFLOWS-$THROUGHPUT.csv ]; then # run query if it hasn't been run already
            run_nexmark_query $NOF_NODES $NOF_DATAFLOWS $THROUGHPUT $QUERY
        fi
    done

    # for testing only :
    # for QUERY in NQ5 NQ5FW NQ5WS NQ8 NQ8FW NQ8WS; do
	# if [ $QUERY = "NQ5" ]; then
	# 	if [ $NOF_NODES -eq 1 ]; then
	# 		if [ $NOF_DATAFLOWS -eq 4 ]; then
	# 			if [ $THROUGHPUT -eq 10000000 ]; then	
	# 				run_nexmark_query $NOF_NODES $NOF_DATAFLOWS $THROUGHPUT $QUERY
	# 			fi
	# 		fi
	# 	fi
	# fi
    # done
}

run_all_throughputs() {
    local NOF_NODES=$1
    local NOF_DATAFLOWS=$2

    # THROUGHPUT=1000 #done : 100K & 1M
    THROUGHPUT=100000
    THROUHGPUT_MAX=1000000

    while [ $THROUGHPUT -le $THROUHGPUT_MAX ]; do
        echo "$NOF_NODES $NOF_DATAFLOWS $THROUGHPUT"

        run_all_queries $NOF_NODES $NOF_DATAFLOWS $THROUGHPUT

        THROUGHPUT=$(( $THROUGHPUT * 10 ))
    done
}

run_all_dataflows() {
    local NOF_NODES=$1

    local NOF_DATAFLOWS_MIN=4
    local NOF_DATAFLOWS_MAX=28

    for NOF_DATAFLOWS in $( seq $NOF_DATAFLOWS_MIN 4 $NOF_DATAFLOWS_MAX )
    do
        run_all_throughputs $NOF_NODES $NOF_DATAFLOWS
    done
}

# This method generates all the bash scripts necessary to 
run_all_nodes() {
    local NOF_NODES_MIN=1
    local NOF_NODES_MAX=4
    # local NOF_NODES_MAX=8

    local NOF_NODES=$NOF_NODES_MIN
    while [ $NOF_NODES -le $NOF_NODES_MAX ]; do
        run_all_dataflows $NOF_NODES
        NOF_NODES=$(( NOF_NODES * 2 ))
    done
}

check_release_folder(){
    if [ ! -f ../../Release_iris/cmake_install.cmake ]; then
        echo "Unable to read CMake configuration in the Release_iris folder. 
        Please make sure that you have configured cmake before running this 
        script next time."
        echo "Trying basic cmake configuration..."
        
        mkdir -p ../../Release_iris

        cd ../../Release_iris
        cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ../  
        make clean
        cd -
    fi
}

check_output_folder(){
    if [ ! -d slurm_exp/observations ]; then
        echo "Observation directory does not exists. Creating directory."

        mkdir -p slurm_exp/observations
    fi
}

load_modules(){
    # make sure the modules are loaded to compile the executables
    if [ $COMPILE_EXECUTABLES -eq 1 ]; then
        module load toolchain/intel
        module load devel/CMake/3.13.3-GCCcore-8.2.0
        module load compiler/GCC/8.2.0-2.31.1
    fi
}

verifications() {
    check_release_folder
    check_output_folder
    load_modules
}

# Main code
# Expected run time =
# nof tries (1) * 
# nof node configurations (4) *
# nof dataflows configurations (7) * (== replaced by 2)
# nof throughput configurations (5)
# nof queries (6) *
# execution time (5 min) +
# nof dataflows configurations (7) *
# nof throughput configurations (5) *
# compilation time (let's say 5 minutes as well, but surely a bit less)
# = 4200 minutes + 175 minutes (1680)
# = 72 hours and 55 minutes (28h)
# ~= 73 hours

# doesn't run the code if we just source the file
RUNNING="$(basename $0)"
if [[ $RUNNING == "nexmark_slurm_exp" ]]; then
    args_check $@
    verifications
    for i in $( seq 1 $NOF_TRIES ); do
        run_all_nodes
    done
fi
