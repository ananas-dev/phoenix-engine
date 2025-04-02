#include "bitboard.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "movegen.h"
#include "position.h"
#include "time.h"
#include "search.h"

void init() {
    movegen_init();
    srand(time(NULL));
}

void debug_game_over(Position *pos) {
    printf("ply: %d\n", pos->ply);
    printf("can_create_king: %d\n", pos->can_create_king);
    printf("kings for current side: %d\n",
           bb_popcnt(pos->pieces[pos->side_to_move][PIECE_KING] & BB_USED));
    printf("kings for other side: %d\n",
           bb_popcnt(pos->pieces[!pos->side_to_move][PIECE_KING] & BB_USED));
    printf("game over: %d\n", position_is_game_over(pos));
}

void search_and_apply_until_depth_1(Position *pos, int depth) {
    if (depth <= 1) {
        return;
    }

    // Search for best move at current depth
    Move best_move = search(pos, depth);

    // Print the move
    char from[3], to[3];
    sq_to_string(best_move.from, from);
    sq_to_string(best_move.to, to);
    printf("Depth %d move: %s -> %s\n", depth, from, to);

    if (best_move.captured) {
        printf("Captures:\n");
        bb_print(best_move.captured);
    }

    // Apply the move
    Position new_pos = make_move(pos, best_move);

    // Print the new position
    position_print(&new_pos);

    // Recurse with depth-1
    search_and_apply_until_depth_1(&new_pos, depth-1);

    debug_game_over(&new_pos);
}

void test() {
    Position pos = position_from_fen("K2S1S2/1SS5/G6k/7g/2S5/S5ss/7s 31 b 10");

    position_print(&pos);

    // MoveList move_list = {0};
    // legal_moves(&pos, &move_list);
    //
    // Move move = search(&pos, 5);
    search_and_apply_until_depth_1(&pos, 5);
    //
    // char from[3];
    // char to[3];
    //
    // sq_to_string(move.from, from);
    // sq_to_string(move.to, to);
    //
    // printf("Move:\n");
    // printf("  from: %s\n", from);
    // printf("  to: %s\n", to);
    // printf("  captures:\n");
    // bb_print(move.captured);
}



Move act(char *position, double time_remaining) {
    Position pos = position_from_fen(position);

    position_print(&pos);

    Move move = search(&pos, 5);

    char from[3];
    char to[3];

    sq_to_string(move.from, from);
    sq_to_string(move.to, to);

    printf("Move:\n");
    printf("  from: %s\n", from);
    printf("  to: %s\n", to);
    // printf("  captures:\n");
    // bb_print(move.captured);

    return move;
}
