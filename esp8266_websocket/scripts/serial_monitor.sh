#!/bin/sh
BUILD_DIR=./build
APP_NAME=websocket-demo

. ./sdkconfig
set -x

python $IDF_PATH/tools/idf_monitor.py\
 --port $ESPPORT --baud $CONFIG_ESPTOOLPY_MONITOR_BAUD\
 --toolchain-prefix $CONFIG_SDK_TOOLPREFIX\
 "$BUILD_DIR/$APP_NAME.elf"
