#!/bin/bash

# number of arguemnts should be 2
if [ $# -ne 2 ]
then
  echo "Usage: $0 <source_pgn_file> <destination_directory>"
  exit 1
fi

# parse argument names to normal names
INPUT_FILE=$1
DEST_DIR=$2

# check if input file exists
if [ ! -f "$INPUT_FILE" ]
then
    echo "Error: File '$INPUT_FILE' does not exist."
    exit 1
fi

# check if dir exists and if not create it
if [ ! -d "$DEST_DIR" ]
then
    mkdir "$DEST_DIR"
    echo "Created directory '$DEST_DIR'."
    exit 1
fi
