#!/bin/bash

# number of arguemnts should be 2
if [ $# -ne 2 ]
then
  echo "Usage: $0 <source_pgn_file> <destination_directory>"
  exit 1
fi
