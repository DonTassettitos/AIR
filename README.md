# AIR - Distributed Dataflow Processing with Asynchronous Iterative Routing

AIR engine is designed from scratch in C++ using the Message Passing Interface (MPI), pthreads for multithreading, and is directly deployed on top of a common HPC workload manager such as SLURM. AIR implements a light-weight, dynamic sharding protocol (referred to as "Asynchronous Iterative Routing"), which facilitates a direct and asynchronous communication among all client nodes and thereby completely avoids the overhead induced by the control flow with a master node that may otherwise form a performance bottleneck. 

A preprint of the paper based on this work can be found here: https://arxiv.org/pdf/2001.00164.pdf

Build & Run:
------------

- Dependencies

An MPI distribution; preferably OpenMPI/MPICH2 

On Mac: 
```sh
  $brew install mpich
```
On Linux: 
```sh
  $sudo apt-get install mpich

Note: If the package mpich has no installation candidate. Do the following:
  $sudo apt-get install build-essential
  $sudo apt-get install gfortran
  $cd /tmp
  $wget http://www.mpich.org/static/downloads/1.4.1/mpich2-1.4.1.tar.gz
  $tar xzvf mpich2-1.4.1.tar.gz  
  $cd mpich2-1.4.1/
  $./configure
  $make
  $sudo make install
```

- Set compilation parameters
```sh
  Edit /path/to/AIR/CMakeLists.txt
```

- Build the project
```sh
  $cd /path/to/AIR/
  $mkdir Release
  $cd Release
  $cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ../
  $make all
```

- Run a use-case
```sh
  $mpirun -np <no.of dataflows> ./AIR <use-case abbreviation> <input throughput>
  (E.g., mpirun -np 4 ./AIR YSB 100)
  ```  
  or
```sh
  $mpiexec -np <no.of dataflows> ./AIR <use-case abbreviation> <input throughput>
```

Running AIR on an HPC Cluster:
------------------------------

The following guidelines are based on the IRIS cluster (https://hpc.uni.lu/systems/iris/) of the University of Luxembourg. 

- Changes in C compiler settings (based on Intel MPI v18.0.1):
```sh
Edit CMakeLists.txt, change "/path/to/bin/mpic++"  to "mpiicpc"
```
>Via Eclipse: Go to the properties>C/C++Build>Settings>[GCCC++Complier>Command & GCCC++Complier>Command], make the above change (and then, clean and build -- you may get mpiicpc missing exception -- and ignore exceptions).
>In case of any change in the src code, replace the remote "src" directory with the new "src" and repeat the compilation/building steps.

- Perform resource allocation -- refer to SLURM documentation (https://hpc.uni.lu/users/docs/slurm.html) for advanced node allocation options.
```sh
  $si
```

- Load the MPI and CMake modules and build the project
```sh
  $module load toolchain/intel
  $module load devel/CMake/3.13.3-GCCcore-8.2.0  
  $mkdir Release_iris
  $cd Release_iris
  $cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ../  
  $make clean
  $make all
```

- Run a use-case:
```sh
  $cd ../AIR/Release_iris/
  $srun -np <no. of dataflows> ./AIR <use-case abbreviation> <throughput>
  (E.g., srun -np 2 ./AIR WA 10000)
```

To run under *OVERCOMMIT* option
```sh
  $srun --overcommit -n <no. of dataflows> ./AIR <use-case abbreviation> <throughput>
```

Available Use-Cases:
--------------------

1. Yahoo! Streaming Benchmark (YSB)

2. Extended Yahoo! Streaming Benchmark (YSBM)

3. Simple Windowed Aggregation (WA)

4. Simple MapReduce (MR)

5. TCP-H Benchmark (TPCH) -- in progress

6. Yet Another Benchmark (YAB) -- in progress

7. NEXMark query 5 using a message counter to aggregate (NQ5)

8. NEXMark query 5 using flow-wrapping to aggregate (NQ5FW)

9. NEXMark query 5 sorting messages before aggregation (NQ5WS)

10. NEXMark query 8 using a message counter to aggregate (NQ8)

11. NEXMark query 8 using flow-wrapping to aggregate (NQ8FW)

12. NEXMark query 8 sorting messages before aggregation (NQ8WS)


Sanity Check Routines:
----------------------

- Postgres SQL queries for sanity checks can be found in Postgres-SanityCheck.sql

- Postgres SQL queries for sanity checks for NEXMark queries can run with `cd AIR/scripts/nexmark && ./run-sanity-checks.sh`

- SANITY_CHECK flag should be set as true to enable this functionality


Note: 
-----

- AGG_WIND_SPAN is by default set as 10sec, you may change that in *AIR/src/communication/Window.hpp* file. This applies for YSB, YSBM, WA, MR, TPCH and YAB.

- For the NEXMark use-cases, the aggregation windspan can be set by changing the parameter *window_duration* in their respective header files :
  - *AIR/src/usecases/NQ5*
  - *AIR/src/usecases/NQ5FW*
  - *AIR/src/usecases/NQ5WS*
  - *AIR/src/usecases/NQ8*
  - *AIR/src/usecases/NQ8FW*
  - *AIR/src/usecases/NQ8WS*

- By default PER_SEC_MSG_COUNT = 1 (If PER_SEC_MSG_COUNT = n >1, a per second input data -- based on the throughput value -- would be generated as n messages. For very high values of throughput, it is advisable to use n>1 to avoid MPI message size limit exceptions.

- By default, DEFAULT_WINDOW_SIZE = 1 000 000. Depending on the number of messages generated per second, the size of the events and the throughput of the dataflow, we need to make sure new messages created when expecting a vertex input have a size big enough to read all the events from the input buffer, but also small enough to take as little time as possible to initialize. This feature is a patch used to fix the issue of having a default message size that is too big when using only one message per second, but there is surely a better solution available.

- *AIR/src/dataflow/Vertex.hpp* has options for setting PIPELINE and SANITY_CHECK flags. 

- *AIR/src/communication/Window.hpp* has options for setting PER_SEC_MSG_COUNT and DEFAULT_WINDOW_SIZE parameters.