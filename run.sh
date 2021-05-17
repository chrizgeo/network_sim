#!/bin/bash
#comipile the two simulators
g++ sim.cpp -o simulator-fcfs -I/usr/include/python3.8 -I/usr/include/python3.8 -I/home/cg/.local/lib/python3.8/site-packages/numpy/core/include  -L/usr/lib/python3.8/config-3.8-x86_64-linux-gnu -L/usr/lib -lpython3.8
g++ sim.cpp -o simulator-prio -DPRIORITY_Q -I/usr/include/python3.8 -I/usr/include/python3.8 -I/home/cg/.local/lib/python3.8/site-packages/numpy/core/include  -L/usr/lib/python3.8/config-3.8-x86_64-linux-gnu -L/usr/lib -lpython3.8
#clear statistics file
echo -e "Qmethod,channelnum,period,deadline,capacity,throughput,utilisation,Queueing Delay,deadlineMissRatio,DMR1,DMR2,DMR3,DMR4,DMR5,DMR6,DMR7,DMR8,DMR9,DMR10,DMR11,DMR12,DMR13,DMR14,DMR15,DMR16,DMR17,DMR18,DMR19,DMR20,DMR21,DMR22,DMR23,DMR24,DMR25,DMR26,DMR27,DMR28,DMR29,DMR30" > statistics.csv 
#run fcfs simulator with different period and deadline
echo fcfs 5000 5000
./simulator-fcfs 5000 5000
echo fcfs 5000 4000
./simulator-fcfs 5000 4000
echo fcfs 5000 3000
./simulator-fcfs 5000 3000
echo fcfs 5000 2000
./simulator-fcfs 5000 2000
echo fcfs 5000 1000
./simulator-fcfs 5000 1000
echo fcfs 4000 5000
./simulator-fcfs 4000 5000
echo fcfs 3000 5000
./simulator-fcfs 3000 5000
echo fcfs 2000 5000
./simulator-fcfs 2000 5000
echo fcfs 1000 5000
./simulator-fcfs 1000 5000
#run priority q simulator with diffferen periods and deadline
echo prio 5000 5000
./simulator-prio 5000 5000
echo prio 5000 4000
./simulator-prio 5000 4000
echo prio 5000 3000
./simulator-prio 5000 3000
echo prio 5000 2000
./simulator-prio 5000 2000
echo prio 5000 1000
./simulator-prio 5000 1000
echo prio 4000 5000
./simulator-prio 4000 5000
echo prio 3000 5000
./simulator-prio 3000 5000
echo prio 2000 5000
./simulator-prio 2000 5000
echo prio 1000 5000
./simulator-prio 1000 5000
echo done