#!/bin/sh
rm -rf $BUILD_ARTIFACT_DIR
mkdir $BUILD_ARTIFACT_DIR
cp --parents $BUILD_APP_DIR/$APP_NAME.bin $BUILD_ARTIFACT_DIR
cp --parents $BUILD_APP_DIR/$APP_NAME.elf $BUILD_ARTIFACT_DIR
cp --parents $BUILD_APP_DIR/$APP_NAME.map $BUILD_ARTIFACT_DIR
cp --parents $BUILD_APP_DIR/bootloader/bootloader.bin $BUILD_ARTIFACT_DIR
cp --parents $BUILD_APP_DIR/bootloader/bootloader.elf $BUILD_ARTIFACT_DIR
cp --parents $BUILD_APP_DIR/bootloader/bootloader.map $BUILD_ARTIFACT_DIR
cp --parents $BUILD_APP_DIR/partition_table/partition-table.bin $BUILD_ARTIFACT_DIR
echo "Copying compiled binaries"

cp $SPIFFS_PARTITION_PATH $BUILD_ARTIFACT_DIR/$SPIFFS_PARTITION_PATH
cp ./partitions.csv $BUILD_ARTIFACT_DIR/partitions.csv
echo "Copying SPIFFS filesystem image"

cp -rf ./scripts $BUILD_ARTIFACT_DIR/scripts
echo "Copying scripts"
