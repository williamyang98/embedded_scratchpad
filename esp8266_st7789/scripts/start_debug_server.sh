#!/bin/sh
set -x
python ./scripts/src/debug_server.py process --executable $BUILD_TESTS_DIR/st7789 --static-dirpath ./static/
