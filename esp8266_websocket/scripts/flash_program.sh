#!/bin/sh
BUILD_DIR=./build
APP_NAME=websocket-demo
SPIFFS_PARTITION="./spiffs_filesystem_partition.bin"

. ./sdkconfig

# Refer to ./partitions.csv for partition offsets and size
python $IDF_PATH/components/esptool_py/esptool/esptool.py\
 --chip $CONFIG_IDF_TARGET\
 --port $ESPPORT --baud $CONFIG_ESPTOOLPY_BAUD\
 write_flash\
 --flash_mode $CONFIG_ESPTOOLPY_FLASHMODE --flash_freq $CONFIG_ESPTOOLPY_FLASHFREQ --flash_size $CONFIG_ESPTOOLPY_FLASHSIZE\
 0x0 "$BUILD_DIR/bootloader/bootloader.bin"\
 0x8000 "$BUILD_DIR/partition_table/partition-table.bin"\
 0x10000 "$BUILD_DIR/$APP_NAME.bin"
