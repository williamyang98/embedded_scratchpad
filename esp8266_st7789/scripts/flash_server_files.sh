#!/bin/sh
OUTPUT_FILE="./spiffs_filesystem_partition.bin"

. ./sdkconfig
set -x

# Refer to ./partitions.csv for partition offsets and size
python $IDF_PATH/components/esptool_py/esptool/esptool.py\
 --chip $CONFIG_IDF_TARGET\
 --port $ESPPORT --baud $CONFIG_ESPTOOLPY_BAUD\
 write_flash\
 --flash_mode $CONFIG_ESPTOOLPY_FLASHMODE --flash_freq $CONFIG_ESPTOOLPY_FLASHFREQ --flash_size $CONFIG_ESPTOOLPY_FLASHSIZE\
 0x110000 "$OUTPUT_FILE"
