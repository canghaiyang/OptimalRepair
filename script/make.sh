#!/bin/sh

cd /home/ych/ec_test/
cmake . 
make
cd /home/ych/ec_test/script
./kill_all_storagenodes.sh 102 105
./scp_storagenodes.sh 102 105
./start_all_storagenodes.sh 102 105