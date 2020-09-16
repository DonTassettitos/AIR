#!/bin/bash
GREEN_COLOR=$(echo "\e[32m")
NOMAL_COLOR=$(echo "\e[m") #doesn't seem to work ... why ?
RED_COLOR=$(echo "\e[31m")

OUTPUT_FILENAME=benchmark.csv
OBSERVATION_TIME=240
WINDOW_DURATION=10
# THROUGHPUT=1000 #events per second

progressbar() {
    sleep 2 # more or less the time to initialize everything
    for i in $( seq 0 $OBSERVATION_TIME ); do
        sleep 1
        printf "\r $i / $OBSERVATION_TIME"
    done
    printf "\r $BENCHMARK_NAME ($NOF_PROCESSES) \n"
}

# make sure the default message size is big enough to run the given troughput
# todo ...

# also make sure cma permission is enabled (depends on system, please set it back to original value after using script)
# wsl -u root
# echo 0 > /proc/sys/kernel/yama/ptrace_scope

# compile program to make sure we are at the latest version
cd ../../Release > /dev/null
cmake -DCMAKE_BUILD_TYPE=Release -Wno-dev -G "CodeBlocks - Unix Makefiles" ../ > /dev/null
make > /dev/null
cd - > /dev/null

# make sure you use the correct message size (1000 msg/sec - 1MB, 10 000 - 10MB)
# or else all benchmarks might segfault.
for THROUGHPUT in 1000 10000
do
    rm $THROUGHPUT.csv &> /dev/null
    for BENCHMARK_NAME in NQ5 NQ5FW NQ5WS NQ8 NQ8FW NQ8WS
    do
        for NOF_PROCESSES in 1 2 4
        do
            progressbar & PID_PROGRESS_BAR=$!

            timeout --kill-after=5 $OBSERVATION_TIME mpiexec -np $NOF_PROCESSES ../../Release/AIR $BENCHMARK_NAME $THROUGHPUT | awk 'NR>3' >> $THROUGHPUT.csv & PID_OBSERVATION=$!
            
            wait $PID_PROGRESS_BAR &>/dev/null
            wait $PID_OBSERVATION &>/dev/null

            # removing useless files
            for i in $( seq 0 $(($NOF_PROCESSES - 1)) ); do
                rm nexmarkgen_bids_$i.tsv
                rm nexmarkgen_auctions_$i.tsv
                rm nexmarkgen_persons_$i.tsv
            done
            rm $BENCHMARK_NAME.tsv &> /dev/null
        done
    done
done