#!/bin/bash
#comipile the two simulators
g++ sim.cpp -o simulator-fcfs -I/usr/include/python3.8 -I/usr/include/python3.8 -I/home/cg/.local/lib/python3.8/site-packages/numpy/core/include  -L/usr/lib/python3.8/config-3.8-x86_64-linux-gnu -L/usr/lib -lpython3.8
g++ sim.cpp -o simulator-prio -DPRIORITY_Q -I/usr/include/python3.8 -I/usr/include/python3.8 -I/home/cg/.local/lib/python3.8/site-packages/numpy/core/include  -L/usr/lib/python3.8/config-3.8-x86_64-linux-gnu -L/usr/lib -lpython3.8
#run fcfs simulator with different period and deadline
./simulator-fcfs 5000 5000
./simulator-fcfs 5000 4000
./simulator-fcfs 5000 3000
./simulator-fcfs 5000 2000
./simulator-fcfs 5000 1000
#run priority q simulator with diffferen periods and deadline
./simulator-prio 5000 5000
./simulator-prio 5000 4000
./simulator-prio 5000 3000
./simulator-prio 5000 2000
./simulator-prio 5000 1000