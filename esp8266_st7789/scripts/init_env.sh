#!/bin/bash
# Usage: bash // This spawns a new shell so we can exit back to original shell
#        source setup_esp8266_environment.sh
echo "Setting up ESP8266-RTOS-SDK"
function export_variable() {
    export $1=$2
    echo "+ export $1=$2"
}

export_variable VENDOR_PATH "$(realpath -s ../vendor)"
export_variable IDF_PATH "$VENDOR_PATH/esp8266-rtos-sdk"
export_variable XTENSA_LX106_PATH "$VENDOR_PATH/xtensa-lx106-elf/bin"
export PATH="$XTENSA_LX106_PATH:$PATH"

# OSTYPE source: https://stackoverflow.com/a/33828925
case "$OSTYPE" in
    linux*|darwin*|msys*|cygwin*)
        source "./venv/bin/activate" ;;
    win*)
        source "./venv/Scripts/activate" ;;
    *)
        echo "ERROR: Unknown OSTYPE: $OSTYPE"
        exit 1
        ;;
esac
echo "Activated python environment"

echo "Exporting toolchain environment variables"
export_variable IDF_TARGET "esp8266"
export_variable ESPPORT "/dev/ttyUSB0"
export_variable BUILD_APP_DIR "$(realpath -s -m ./build/app)"
export_variable BUILD_TESTS_DIR "$(realpath -s -m ./build/tests)"
export_variable BUILD_ARTIFACT_DIR "$(realpath -s -m ./build-artifact)"
export_variable STATIC_FILES_DIR "$(realpath -s ./static)"
export_variable SPIFFS_PARTITION_PATH "$(realpath -s -m ./spiffs_filesystem_partition.bin)"
export_variable SDKCONFIG_PATH "$(realpath -s -m ./sdkconfig)"
export_variable APP_NAME "websocket-demo"
