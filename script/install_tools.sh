#!/bin/sh
echo "intall tools"
CURRENT_DIR=$(pwd)
PARENT_DIR=$(dirname "$(pwd)")
TOOLS_DIR=$PARENT_DIR/tools
IP_PREFIX=192.168.7.

for i in $(seq $1 $2)
do
{
printf "install iperf3, wondershaper, nasm-2.15, isa-l ....: root@192.168.7.%d\n" $i
ssh $IP_PREFIX$i "cd $TOOLS_DIR && unzip -oq iperf-master.zip && unzip -oq wondershaper-master.zip &&unzip -oq nasm-2.15.zip && unzip -oq isa-l.zip"
ssh $IP_PREFIX$i "cd $TOOLS_DIR/iperf-master && ./configure && ldconfig /usr/local/lib && make && make install"
ssh $IP_PREFIX$i "cd $TOOLS_DIR/nasm-2.15 && ./configure && make && make install"
ssh $IP_PREFIX$i "cd $TOOLS_DIR/isa-l && ./autogen.sh && ./configure && make && sudo make install"
} &
done
wait

