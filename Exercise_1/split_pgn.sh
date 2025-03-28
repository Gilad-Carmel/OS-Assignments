#!/bin/bash

# number of arguments should be 2
validate_arguments() {
    if [ $# -ne 2 ]; then
        echo "Usage: $0 <source_pgn_file> <destination_directory>"
        exit 1
    fi
}

# check if the input file exists
check_input_file() {
    local input_file=$1
    if [ ! -f "$input_file" ]; then
        echo "Error: File '$input_file' does not exist."
        exit 1
    fi
}

# create destination directory
ensure_directory_exists() {
    local dest_dir=$1
    if [ ! -d "$dest_dir" ]; then
        mkdir "$dest_dir"
        echo "Created directory '$dest_dir'."
    fi
}

# Main script logic
validate_arguments "$@"
INPUT_FILE=$1
DEST_DIR=$2
check_input_file "$INPUT_FILE"
ensure_directory_exists "$DEST_DIR"
