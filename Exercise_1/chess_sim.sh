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
read_pgn_file() {
    local input_file=$1
    local content=""
    
    echo "Metadata from PGN file:"
    while read -r line || [ -n "$line" ]; do
        if [[ $line = "["* ]]; then
            echo "$line"
        else
            content+="$line"$'\s'
        fi
    done < "$input_file"
    echo ""
    uci=$(python3 parse_moves.py "$content")

}

declare -A board # using declare for 2d array
# initialize board
init_board() {
    for ((row=0; row<8; row++)); do
        for ((col=0; col<8; col++)); do
            board[$row,$col]="."
        done
    done

    board[0,0]="r"
    board[0,1]="n"
    board[0,2]="b"
    board[0,3]="q"
    board[0,4]="k"
    board[0,5]="b"
    board[0,6]="n"
    board[0,7]="r"
    for ((col=0; col<8; col++)); do
        board[1,$col]="p"
    done

    board[7,0]="R"
    board[7,1]="N"
    board[7,2]="B"
    board[7,3]="Q"
    board[7,4]="K"
    board[7,5]="B"
    board[7,6]="N"
    board[7,7]="R"
    for ((col=0; col<8; col++)); do
        board[6,$col]="P"
    done
}


# print board

print_board() {
    echo "  a b c d e f g h"
    for ((row=0; row<8; row++)); do
        echo -n "$((8-row)) " # print row number
        for ((col=0; col<8; col++)); do
            echo -n "${board[$row,$col]} " # print board content at (row,col)
        done
        echo "$((8-row))"
    done
    echo "  a b c d e f g h"
}

# implement move logic
get_file_col() {
    local file=$1
    local file_mapping=("a" "b" "c" "d" "e" "f" "g" "h")
    for ((i=0; i<8; i++)); do
        if [[ ${file_mapping[$i]} == "$file" ]]; then
            echo $i
            return
        fi
    done
}

parse_uci_to_move() {
    local uci_notation=$1
    local file=${uci_notation:0:1}
    local rank=${uci_notation:1:1}
    local col=$(get_file_col "$file")
    local row=$((8 - rank))
    echo "$row $col"
}

# move
move_piece() {
    local move_uci=$1
    local start_uci=${move_uci:0:2}
    local end_uci=${move_uci:2:2}
    local promotion=${move_uci:4:1}
    read -r start_row start_col <<< "$(parse_uci_to_move "$start_uci")"
    read -r to_row to_col <<< "$(parse_uci_to_move "$end_uci")"
    local piece=${board[$start_row,$start_col]}

    board[$to_row,$to_col]=$piece
    board[$start_row,$start_col]="."

    # implement promotion
    if [[ $promotion != "" ]]; then
        if [[ $piece == "P" ]]; then
            if [[ $promotion == "q" ]]; then
                board[$to_row,$to_col]="Q"
            elif [[ $promotion == "r" ]]; then
                board[$to_row,$to_col]="R"
            elif [[ $promotion == "b" ]]; then
                board[$to_row,$to_col]="B"
            elif [[ $promotion == "n" ]]; then
                board[$to_row,$to_col]="N"
            fi
        elif [[ $piece == "p" ]]; then
            if [[ $promotion == "q" ]]; then
                board[$to_row,$to_col]="q"
            elif [[ $promotion == "r" ]]; then
                board[$to_row,$to_col]="r"
            elif [[ $promotion == "b" ]]; then
                board[$to_row,$to_col]="b"
            elif [[ $promotion == "n" ]]; then
                board[$to_row,$to_col]="n"
            fi
        fi
    fi


    # implement castling
    if [[ $piece == "k" && $start_row == 0 && $start_col == 4 ]]; then
        if [[ $to_row == 0 && $to_col == 6 ]]; then
            board[0,5]="r"
            board[0,7]="."
        elif [[ $to_row == 0 && $to_col == 2 ]]; then
            board[0,3]="r"
            board[0,0]="."
        fi
    
    elif [[ $piece == "K" && $start_col == 4 && $start_row == 7 ]]; then
        if [[ $to_row == 7 && $to_col == 6 ]]; then
            board[7,5]="R"
            board[7,7]="."
        elif [[ $to_row == 7 && $to_col == 2 ]]; then
            board[7,3]="R"
            board[7,0]="."
        fi
    fi

    # implement en passant
    if [[ ($piece == "p" || $piece == "P") && $start_col != "$to_col" && ${board[$to_row,$to_col]} == "." ]]; then
        if [[ $piece == "p" ]]; then
            board[$((to_row-1)),$to_col]="."
        else
            board[$((to_row+1)),$to_col]="."
        fi
    fi
}



# main
validate_arguments "$@"
INPUT_FILE=$1
check_input_file "$INPUT_FILE"
read_pgn_file "$INPUT_FILE"
# moves array
read -a moves_history <<< "$uci"
init_board

current_move=0
echo "Move $current_move/${#moves_history[@]}"
print_board
while true; do
    echo -n "Press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit:"

    read key
    
    if [[ $key == "d" ]]; then
        if [[ $current_move -ge ${#moves_history[@]} ]]; then
            echo ""
            echo "No more moves available."
            continue
        fi
        # move logic
        move_piece "${moves_history[$current_move]}"
        ((current_move++))

    elif [[ $key == "a" ]]; then
        if [[ $current_move -gt 0 ]]; then
            ((current_move--))
            init_board
            
            for ((m=0; m<current_move; m++)); do
                move_piece "${moves_history[$m]}"
            done
        fi

    elif [[ $key == "w" ]]; then
        current_move=0
        init_board

    elif [[ $key == "s" ]]; then
        current_move=0
        init_board
        while [[ $current_move -lt ${#moves_history[@]} ]]; do
            move_piece "${moves_history[$current_move]}"
            ((current_move++))
        done
        
    elif [[ $key == "q" ]]; then
        echo ""
        echo "Exiting."
        echo "End of game."
        exit 0
    else
        echo ""
        echo "Invalid key pressed: $key"
        continue
    fi
    echo ""
    echo "Move $current_move/${#moves_history[@]}"
    print_board

done