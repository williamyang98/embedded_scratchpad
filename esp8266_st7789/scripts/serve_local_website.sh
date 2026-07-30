#!/bin/sh
set -x
python -m http.server -d $STATIC_FILES_DIR

