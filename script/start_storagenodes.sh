#!/bin/sh
echo "start storagenodes..."
PARENT_DIR=$(dirname "$1")
LOG_DIR=$PARENT_DIR/log
BUILD_DIR=$PARENT_DIR/build

nohup $BUILD_DIR/storagenodes > $LOG_DIR/log.txt 2>&1 &





