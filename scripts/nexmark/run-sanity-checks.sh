#!/bin/bash
GREEN_COLOR=$(echo "\e[32m")
NOMAL_COLOR=$(echo "\e[m") #doesn't seem to work ... why ?
RED_COLOR=$(echo "\e[31m")

OUTPUT_FILENAME=benchmark.csv
OBSERVATION_TIME=20
WINDOW_DURATION=10
THROUGHPUT=1000 #events per second

progressbar() {
    sleep 2 # more or less the time to initialize everything
    for i in $( seq 0 $OBSERVATION_TIME ); do
        sleep 1
        printf "\r $i / $OBSERVATION_TIME"
    done
    printf "\r $BENCHMARK_NAME ($NOF_PROCESSES) "
}

# before starting script, please check that sanity checks are activated
# todo ... 

# compile program to make sure we are at the latest version
cd ../../Release > /dev/null
make > /dev/null
cd - > /dev/null

# activate postresql
sudo service postgresql start > /dev/null

for BENCHMARK_NAME in NQ5 NQ5FW NQ5WS NQ8 NQ8FW NQ8WS
do
    rm -r $BENCHMARK_NAME &>/dev/null
    mkdir $BENCHMARK_NAME &>/dev/null
    for NOF_PROCESSES in 1 2 4
    do
        progressbar & PID_PROGRESS_BAR=$!

        timeout --kill-after=5 $OBSERVATION_TIME mpiexec -np $NOF_PROCESSES ../../Release/AIR $BENCHMARK_NAME $THROUGHPUT &>/dev/null & PID_OBSERVATION=$!
        
        wait $PID_PROGRESS_BAR &>/dev/null
        wait $PID_OBSERVATION &>/dev/null

        # preparing data files to read from
        for i in $( seq 0 $(($NOF_PROCESSES - 1)) ); do
            cat nexmarkgen_bids_$i.tsv >> $BENCHMARK_NAME/generator-bids.tsv && rm nexmarkgen_bids_$i.tsv
            cat nexmarkgen_auctions_$i.tsv >> $BENCHMARK_NAME/generator-auctions.tsv && rm nexmarkgen_auctions_$i.tsv
            cat nexmarkgen_persons_$i.tsv >> $BENCHMARK_NAME/generator-persons.tsv && rm nexmarkgen_persons_$i.tsv
        done
        mv $BENCHMARK_NAME.tsv $BENCHMARK_NAME/collector.tsv

        # run script
        if [[ $BENCHMARK_NAME == NQ8* ]]
        then
            QUERY="NQ8"
        elif [[ $BENCHMARK_NAME == NQ5* ]]
        then
            QUERY="NQ5"
        else
            exit 0
        fi

        DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
        COMMAND=$(m4 -D BENCHMARK_NAME="$BENCHMARK_NAME" DIR="$DIR" nexmark-sanity-check-$QUERY.sql)
        rm command.sql &>/dev/null
        echo "$COMMAND" >> command.sql
        sudo -u postgres psql -f command.sql &>/dev/null
        rm command.sql

        # check if results are correct
        if test `sudo -u postgres psql -t -c "select count(*) - count(window_id) from comparison;"` = 0
        then 
            printf "${GREEN_COLOR} OK \n\e[m";
        else
            printf "${RED_COLOR} ERROR \n\e[m";
            exit 0;
        fi
    done
    # cleanup
    rm -r $BENCHMARK_NAME &>/dev/null
done

# shut down postgresql
sudo service postgresql stop > /dev/null