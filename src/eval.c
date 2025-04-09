#include "eval.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitboard.h"
#include "movegen.h"

int soldier_position_reward[NUM_SQUARE] = {
    20, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 10, 10, 0, 0, 0,
    0, 0, 20, 50, 50, 50, 0, 0,
    0, 0, 20, 50, 50, 20, 0, 0,
    0, 0, 50, 50, 50, 20, 0, 0,
    0, 0, 0, 10, 10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 20,
};

const int soldier_center_reward[NUM_SQUARE] = {
    3, 1, 2, 2, 2, 2, 1, 0,
    2, 2, 3, 3, 3, 3, 2, 1,
    2, 3, 4, 4, 4, 4, 3, 2,
    2, 3, 4, 5, 5, 4, 3, 2,
    2, 3, 4, 4, 4, 4, 3, 2,
    1, 2, 3, 3, 3, 3, 2, 1,
    0, 1, 2, 2, 2, 2, 1, 3,
};

const int center_reward[NUM_SQUARE] = {
    -1, 1, 2, 2, 2, 2, 1, -1,
    1, 2, 3, 3, 3, 3, 2, 1,
    2, 3, 4, 4, 4, 4, 3, 2,
    2, 3, 4, 5, 5, 4, 3, 2,
    2, 3, 4, 4, 4, 4, 3, 2,
    1, 2, 3, 3, 3, 3, 2, 1,
    -1, 1, 2, 2, 2, 2, 1, -1,
};

void eval_init(State *state) {
    for (Square i = SQ_A1; i <= SQ_H7; i++) {
        for (Square j = SQ_A1; j <= SQ_H7; j++) {
            state->distance[i][j] = 13 - (abs(sq_file(i) - sq_file(j)) + abs(sq_rank(i) - sq_rank(j)));
        }
    }
}

static inline int mobility(State *state, Position *position, Color color, Bitboard color_pieces, Bitboard all_pieces) {
    Bitboard general_mobility = BB_EMPTY;
    Bitboard king_mobility = BB_EMPTY;

    Bitboard general_it = position->pieces[color][PIECE_GENERAL] & BB_USED;
    while (general_it) {
        Square from = bb_it_next(&general_it);
        general_mobility |= general_attacks(state, from, all_pieces) & ~color_pieces;
    }

    Bitboard king_it = position->pieces[color][PIECE_KING] & BB_USED;
    while (king_it) {
        Square from = bb_it_next(&king_it);
        king_mobility |= king_attacks(state, from) & ~color_pieces;
    }

    return 8 * bb_popcnt(general_mobility) + 1 * bb_popcnt(king_mobility);
}



int eval(State *state, Position *position) {
    int turn = position->side_to_move == COLOR_WHITE ? 1 : -1;

    int material_score = 0;

    int num_white_solider = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
    int num_white_general = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_GENERAL]);
    int num_white_king = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_KING]);
    int num_black_solider = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
    int num_black_general = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_GENERAL]);
    int num_black_king = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_KING]);

    Bitboard white_pieces = pieces_by_color(position, COLOR_WHITE) & BB_USED;
    Bitboard black_pieces = pieces_by_color(position, COLOR_BLACK) & BB_USED;
    Bitboard all_pieces = white_pieces | black_pieces;

    Square white_king_pos = -1;
    Square black_king_pos = -1;

    // Precalculate king positions
    Bitboard white_king_it = position->pieces[COLOR_WHITE][PIECE_KING] & BB_USED;
    if (white_king_it) {
        white_king_pos = bb_it_next(&white_king_it);
    }

    Bitboard black_king_it = position->pieces[COLOR_BLACK][PIECE_KING] & BB_USED;
    if (black_king_it) {
        black_king_pos = bb_it_next(&black_king_it);
    }

    // Don't care about material in setup phase
    if (position->ply >= 10) {
        material_score = 10000 * (num_white_king - num_black_king)
                         + 900 * (num_white_general - num_black_general)
                         + 100 * (num_white_solider - num_black_solider);
    }

    int white_mobility = mobility(state, position, COLOR_WHITE, white_pieces, all_pieces);
    int black_mobility = mobility(state, position, COLOR_BLACK, black_pieces, all_pieces);

    int mobility_score = white_mobility - black_mobility;

    int white_soldier_position_score = 0;
    Bitboard white_soldier_it = position->pieces[COLOR_WHITE][PIECE_SOLDIER] & BB_USED;
    while (white_soldier_it) {
        Square from = bb_it_next(&white_soldier_it);
        white_soldier_position_score += soldier_center_reward[from] * 5;
    }

    int black_soldier_position_score = 0;
    Bitboard black_soldier_it = position->pieces[COLOR_BLACK][PIECE_SOLDIER] & BB_USED;
    while (black_soldier_it) {
        Square from = bb_it_next(&black_soldier_it);
        black_soldier_position_score += soldier_center_reward[from] * 5;
    }

    int soldier_position_score = white_soldier_position_score - black_soldier_position_score;

    int white_king_protection = 0;
    int black_king_protection = 0;

    if (num_white_king > 0) {
        int cnt = bb_popcnt(king_attacks(state, white_king_pos) & white_pieces);
        if (cnt <= 2) {
            white_king_protection = 10 * cnt;
        } else {
            // After 2 pieces, each additional piece contributes less
            white_king_protection = 10 * 2 + (cnt - 2) * 5;
        }
    }
    if (num_black_king > 0) {
        int cnt = bb_popcnt(king_attacks(state, black_king_pos) & black_pieces);
        if (cnt <= 2) {
            black_king_protection = 10 * cnt;
        } else {
            // After 2 pieces, each additional piece contributes less
            black_king_protection = 10 * 2 + 5 * (cnt - 2);
        }
    }

    int king_protection_score = white_king_protection - black_king_protection;

    // Endgame
    int lone_king_distance_white = 0;
    int lone_king_distance_black = 0;

    int lone_king_center_reward_white = 0;
    int lone_king_center_reward_black = 0;

    if (position->ply >= 10) {
        if (num_black_king > 0 && num_white_king > 0) {
            if (num_black_general == 0 && num_white_general > 0) {
                lone_king_distance_white = state->distance[white_king_pos][black_king_pos] * 30;
                lone_king_center_reward_black = center_reward[black_king_pos] * 10;
            }

            if (num_white_general == 0 && num_black_general > 0) {
                lone_king_distance_black = state->distance[white_king_pos][black_king_pos] * 30;
                lone_king_center_reward_white = center_reward[white_king_pos] * 10;
            }
        }
    }

    int lone_king_distance_score = lone_king_distance_white - lone_king_distance_black;
    int lone_king_center_score = lone_king_center_reward_white - lone_king_center_reward_black;

    return (
               material_score
               + mobility_score
               + soldier_position_score
               + king_protection_score
               + lone_king_distance_score
               + lone_king_center_score
           ) * turn;
}
