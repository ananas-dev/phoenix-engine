#include "eval.h"

#include <stdlib.h>

#include "bitboard.h"
#include "movegen.h"

int distance[NUM_SQUARE][NUM_SQUARE];

int soldier_position_reward[NUM_SQUARE] = {
    20, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 10, 10, 0, 0, 0,
    0, 0, 20, 20, 200, 200, 0, 0,
    0, 0, 20, 200, 200, 20, 0, 0,
    0, 0, 200, 200, 20, 20, 0, 0,
    0, 0, 0, 10, 10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 20,
};

void eval_init() {
    for (Square i = SQ_A1; i <= SQ_H7; i++) {
        for (Square j = SQ_A1; j <= SQ_H7; j++) {
            distance[i][j] = 13 - abs(sq_file(i) - sq_file(j)) + abs(sq_rank(i) - sq_rank(j));
        }
    }
}

static inline int mobility(Position *position, Color color, Bitboard color_pieces, Bitboard all_pieces) {
    Bitboard general_mobility = BB_EMPTY;
    Bitboard king_mobility = BB_EMPTY;

    Bitboard general_it = position->pieces[color][PIECE_GENERAL] & BB_USED;
    while (general_it) {
        Square from = bb_it_next(&general_it);
        general_mobility |= general_attacks(from, all_pieces) & ~color_pieces;
    }

    Bitboard king_it = position->pieces[color][PIECE_KING] & BB_USED;
    while (king_it) {
        Square from = bb_it_next(&king_it);
        king_mobility |= king_attacks(from) & ~color_pieces;
    }

    return 16 * bb_popcnt(general_mobility) + 1 * bb_popcnt(king_mobility);
}

int eval(Position *position) {
    int turn = position->side_to_move == COLOR_WHITE ? 1 : -1;

    int material_score = 0;

    // Don't care about material in setup phase
    if (position->ply >= 10) {
        int num_white_solider = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
        int num_white_general = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_GENERAL]);
        int num_white_king = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_KING]);
        int num_black_solider = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
        int num_black_general = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_GENERAL]);
        int num_black_king = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_KING]);

        material_score = 900 * (num_white_general - num_black_general)
                         + 100 * (num_white_solider - num_black_solider);
    }

    Bitboard white_pieces = pieces_by_color(position, COLOR_WHITE) & BB_USED;
    Bitboard black_pieces = pieces_by_color(position, COLOR_BLACK) & BB_USED;
    Bitboard all_pieces = white_pieces | black_pieces;

    int white_mobility = mobility(position, COLOR_WHITE, white_pieces, all_pieces);
    int black_mobility = mobility(position, COLOR_BLACK, black_pieces, all_pieces);

    int mobility_score = white_mobility - black_mobility;

    int white_soldier_position_score = 0;
    Bitboard white_soldier_it = position->pieces[COLOR_WHITE][PIECE_SOLDIER] & BB_USED;
    while (white_soldier_it) {
        Square from = bb_it_next(&white_soldier_it);
        white_soldier_position_score += soldier_position_reward[from];
    }

    int black_soldier_position_score = 0;
    Bitboard black_soldier_it = position->pieces[COLOR_BLACK][PIECE_SOLDIER] & BB_USED;
    while (black_soldier_it) {
        Square from = bb_it_next(&black_soldier_it);
        black_soldier_position_score += soldier_position_reward[from];
    }

    int soldier_position_score = white_soldier_position_score - black_soldier_position_score;

    Bitboard white_king_it = position->pieces[COLOR_WHITE][PIECE_KING] & BB_USED;
    int white_king_protection = 0;

    if (white_king_it) {
        Square sq = bb_it_next(&white_king_it);
        white_king_protection = 15 * bb_popcnt(king_attacks(sq) & white_pieces);
    }

    Bitboard black_king_it = position->pieces[COLOR_BLACK][PIECE_KING] & BB_USED;
    int black_king_protection = 0;

    if (black_king_it) {
        Square sq = bb_it_next(&white_king_it);
        int cnt = bb_popcnt(king_attacks(sq) & black_pieces);
        if (cnt <= 2) {
            black_king_protection = 15 * cnt;
        } else {
            black_king_protection = 15 * 2 + (cnt - 2) * 7; // After 2 pieces, each additional piece contributes less
        }
    }

    int king_protection_score = white_king_protection - black_king_protection;

    return (material_score + mobility_score + soldier_position_score + king_protection_score) * turn;
};
