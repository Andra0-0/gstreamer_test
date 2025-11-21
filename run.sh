#!/bin/sh

LD_LIBRARY_PATH=/home/cat/multimedia_matrix/lib GST_VIDEO_CONVERT_USE_RGA=1 GST_VIDEO_CONVERT_ONLY_RGA3=1 GST_DEBUG_DUMP_DOT_DIR=$(pwd)/dot GST_DEBUG=0 ./matrix/matrix
