#!/bin/sh

cd /home/ych/ec_test/
cmake . 
make
cd /home/ych/ec_test/script
./kill_all_storagenodes.sh 102 106
./scp_storagenodes.sh 102 106
./start_all_storagenodes.sh 102 106