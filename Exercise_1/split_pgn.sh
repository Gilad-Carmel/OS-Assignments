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

split_games() {
    local input_file=$1
    local dest_dir=$2
    local game_no=0
    local file_name="${input_file##*/}" # removes everything in path but file name
    local file_name="${file_name%.pgn}" # remove the ending
    local output_file=""
    local buffer=""
    
    while IFS= read -r line || [ -n "$line" ]; do
        if [[ $line = "[Event "* ]]; then
            # check if buffer has content
            if [ -n "$buffer" ]; then
                buffer="${buffer%$'\n'}" # remove whitespaces
                echo -n "$buffer" > "$output_file"
            fi
            
            ((game_no++))
            output_file="$dest_dir/${file_name}_${game_no}.pgn"
            echo "Saved game to $output_file"
            
            buffer="$line"
        elif [ -n "$output_file" ]; then
            # add a newline before appending if buffer is not empty
            if [ -n "$buffer" ]; then
                buffer="$buffer"$'\n'"$line"
            else
                buffer="$line"
            fi
        fi
    done < "$input_file"
    
    # save the last game buffer
    if [ -n "$buffer" ]; then
        # remove empty line
        buffer="${buffer%$'\n'}"
        # write with no newline
        echo -n "$buffer" > "$output_file"
    fi
    
    echo "All games have been split and saved to '$dest_dir'."
}

# Main script logic
validate_arguments "$@"
INPUT_FILE=$1
DEST_DIR=$2
check_input_file "$INPUT_FILE"
ensure_directory_exists "$DEST_DIR"
split_games "$INPUT_FILE" "$DEST_DIR"