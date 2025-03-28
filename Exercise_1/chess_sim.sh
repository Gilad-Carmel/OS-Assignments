#!/bin/bash

# validate arguments
validate_arguments() {
    if [ $# -ne 1 ]; then
        echo "Usage: $0 <PGN_FILE>"
        exit 1
    fi
}

# check if file exists
check_input_file() {
    local input_file=$1
    if [ ! -f "$input_file" ]; then
        echo "File does not exist: $input_file"
        exit 1
    fi
}

# read file and split pgn parts (metadata and game)


# print metadata

# print board

# implement basic moves

# move counter and interactive ("Press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit:")
# edge cases: 
# - if d is pressed at the end print "No more moves available."
# - if a is pressed at the beginning - nothing happens

# implement promotion

# BONUS implement en passant and castling

# main
validate_arguments "$@"
INPUT_FILE=$1
check_input_file "$INPUT_FILE"


# this should be a function
content=""
echo "Metadata from PGN file:"
while read -r line || [ -n "$line" ]; do
    if [[ $line = "["* ]]; then
        echo "$line"
    else
        content+="$line"$'\s'
    fi
done < "$INPUT_FILE"

# this should be a function
echo "Game content:"
uci=$(python3 parse_moves.py "$content")
echo "$uci"

# function to print chess board
print_board() {
    local board=(
        "rnbqkbnr"
        "pppppppp"
        "........"
        "........"
        "........"
        "........"
        "PPPPPPPP"
        "RNBQKBNR"
    )
    for row in "${board[@]}"; do
        echo "$row"
    done
}

print_board