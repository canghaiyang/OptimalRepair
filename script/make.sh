#!/bin/sh

cd /home/ych/OptimalRepair/
cmake . 
make
cd /home/ych/OptimalRepair/script
./kill_all_storagenodes.sh 102 106
./scp_storagenodes.sh 102 106
./start_all_storagenodes.sh 102 106