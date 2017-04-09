#!/bin/bash
export DATA_SIZE="10B"
export INPUT_DATA="/dev/urandom"
export OUTPUT_DATA="/Users/eitanantler/operating_systems/output_test.txt"

./data_filter $DATA_SIZE $INPUT_DATA $OUTPUT_DATA
