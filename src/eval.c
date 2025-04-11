#include "eval.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboard.h"
#include "movegen.h"

const int center_table[NUM_SQUARE] = {
    0, 1, 2, 2, 2, 2, 1, 0,
    1, 2, 3, 3, 3, 3, 2, 1,
    2, 3, 4, 4, 4, 4, 3, 2,
    2, 3, 4, 5, 5, 4, 3, 2,
    2, 3, 4, 4, 4, 4, 3, 2,
    1, 2, 3, 3, 3, 3, 2, 1,
    0, 1, 2, 2, 2, 2, 1, 0,
};

static const int edge_table[NUM_SQUARE] = {
    2, 1, 1, 1, 1, 1, 1, 2,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    2, 1, 1, 1, 1, 1, 1, 2,
};

void square_structure_init(State *state) {
    for (File file = FILE_A; file <= FILE_G; file++) {
        for (Rank rank = RANK_1; rank <= RANK_6; rank++) {
            Bitboard square_structure = BB_EMPTY;
            square_structure |= bb_from_sq(sq_get(file, rank));
            square_structure |= bb_from_sq(sq_get(file+1, rank));
            square_structure |= bb_from_sq(sq_get(file+1, rank+1));
            square_structure |= bb_from_sq(sq_get(file, rank+1));

            state->square_structure_table[file][rank] = square_structure;
        }
    }
}

int count_square_structures(State *state, Position *position) {
    int count = 0;

    for (File file = FILE_A; file <= FILE_G; file++) {
        for (Rank rank = RANK_1; rank <= RANK_6; rank++) {
            Bitboard square_structure = state->square_structure_table[file][rank];

            if (bb_popcnt(square_structure & position->pieces[COLOR_WHITE][PIECE_SOLDIER]) == 4) {
                count++;
            }

            if (bb_popcnt(square_structure & position->pieces[COLOR_BLACK][PIECE_SOLDIER]) == 4) {
                count--;
            }
        }
    }

    return count;
}

void eval_init(State *state) {
    memset(&state->weights, 0, sizeof(EvalWeights));
    // state->weights = (EvalWeights){
    //     .general_phase = 4,
    //     .soldier_phase = 1,
    //     .general_material = 900,
    //     .king_mobility = 1,
    //     .general_mobility = 8,
    //     .soldier_mobility = 0,
    //     .soldier_center = 5,
    //     .king_shelter = 10,
    // };

    for (Square i = SQ_A1; i <= SQ_H7; i++) {
        for (Square j = SQ_A1; j <= SQ_H7; j++) {
            state->distance[i][j] = 13 - (abs(sq_file(i) - sq_file(j)) + abs(sq_rank(i) - sq_rank(j)));
        }
    }

    square_structure_init(state);
}

int eval(State *state, Position *position) {
    int turn = position->side_to_move == COLOR_WHITE ? 1 : -1;

    int num_white_soldier = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
    int num_white_general = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_GENERAL]);
    int num_white_king = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_KING]);
    int num_black_soldier = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
    int num_black_general = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_GENERAL]);
    int num_black_king = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_KING]);

    // Precalculate king positions
    Square white_king_pos = -1;
    Square black_king_pos = -1;

    Bitboard w_king_it = position->pieces[COLOR_WHITE][PIECE_KING] & BB_USED;
    if (w_king_it) {
        white_king_pos = bb_it_next(&w_king_it);
    }

    Bitboard b_king_it = position->pieces[COLOR_BLACK][PIECE_KING] & BB_USED;
    if (b_king_it) {
        black_king_pos = bb_it_next(&b_king_it);
    }

    Bitboard white_pieces = pieces_by_color(position, COLOR_WHITE) & BB_USED;
    Bitboard black_pieces = pieces_by_color(position, COLOR_BLACK) & BB_USED;
    Bitboard all_pieces = white_pieces | black_pieces;

    int square_structures = count_square_structures(state, position);

    int ss_pairs = 0;
    int sg_pairs = 0;

    int edge_pieces = 0;

    int king_threats = 0;

    int soldier_mobility = 0;
    int soldier_center = 0;
    int soldier_king_dist = 0;

    Bitboard w_controlled_squares = BB_EMPTY;
    Bitboard b_controlled_squares = BB_EMPTY;

    // SOLDIER ITER

    Bitboard w_soldier_it = position->pieces[COLOR_WHITE][PIECE_SOLDIER] & BB_USED;
    while (w_soldier_it) {
        Square from = bb_it_next(&w_soldier_it);
        Bitboard attacks = soldier_attacks(state, from);
        w_controlled_squares |= attacks;

        ss_pairs += bb_popcnt(soldier_attacks(state, from) & position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
        sg_pairs += bb_popcnt(soldier_attacks(state, from) & position->pieces[COLOR_WHITE][PIECE_GENERAL]);
        soldier_mobility += bb_popcnt(attacks & ~all_pieces);

        edge_pieces += edge_table[from];
        soldier_center += center_table[from];

        if (num_black_king > 0) {
            soldier_king_dist += state->distance[from][black_king_pos];
        }

        king_threats += bb_popcnt(attacks & position->pieces[COLOR_BLACK][PIECE_KING]);
    }

    Bitboard b_soldier_it = position->pieces[COLOR_BLACK][PIECE_SOLDIER] & BB_USED;
    while (b_soldier_it) {
        Square from = bb_it_next(&b_soldier_it);
        Bitboard attacks = soldier_attacks(state, from);
        b_controlled_squares |= attacks;

        ss_pairs -= bb_popcnt(soldier_attacks(state, from) & position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
        sg_pairs -= bb_popcnt(soldier_attacks(state, from) & position->pieces[COLOR_BLACK][PIECE_GENERAL]);
        soldier_mobility -= bb_popcnt(attacks & ~all_pieces);

        edge_pieces -= edge_table[from];
        soldier_center -= center_table[from];

        if (num_white_king > 0) {
            soldier_king_dist -= state->distance[from][white_king_pos];
        }

        king_threats -= bb_popcnt(attacks & position->pieces[COLOR_WHITE][PIECE_KING]);
    }

    // GENERAL ITER

    int general_mobility = 0;

    Bitboard w_general_it = position->pieces[COLOR_WHITE][PIECE_GENERAL] & BB_USED;
    while (w_general_it) {
        Square from = bb_it_next(&w_general_it);
        Bitboard attacks = general_attacks(state, from, all_pieces);
        w_controlled_squares |= attacks;
        general_mobility += bb_popcnt(attacks & ~white_pieces);

        king_threats += bb_popcnt(attacks & position->pieces[COLOR_BLACK][PIECE_KING]);

        edge_pieces += edge_table[from];
    }

    Bitboard b_general_it = position->pieces[COLOR_BLACK][PIECE_GENERAL] & BB_USED;
    while (b_general_it) {
        Square from = bb_it_next(&b_general_it);
        Bitboard attacks = general_attacks(state, from, all_pieces);
        b_controlled_squares |= attacks;
        general_mobility -= bb_popcnt(attacks & ~black_pieces);

        king_threats -= bb_popcnt(attacks & position->pieces[COLOR_WHITE][PIECE_KING]);

        edge_pieces += edge_table[from];
    }

    // KING ITER

    int king_mobility = 0;
    int king_shelter = 0;
    int king_chase = 0;
    int king_center = 0;

    if (num_white_king > 0 && num_black_king > 0) {
        int kings_distance = state->distance[white_king_pos][black_king_pos];
        if (num_black_general == 0 && num_white_general > 0) {
            king_chase += kings_distance;
        } else if (num_white_general == 0 && num_black_general > 0) {
            king_chase -= kings_distance;
        }
    }


    if (num_white_king > 0) {
        Bitboard attacks = king_attacks(state, white_king_pos);
        w_controlled_squares |= attacks;
        king_mobility += bb_popcnt(attacks & ~all_pieces);
        king_shelter += bb_popcnt(attacks & white_pieces);

        edge_pieces += edge_table[white_king_pos];
        king_center += center_table[white_king_pos];
    }

    if (num_black_king > 0) {
        Bitboard attacks = king_attacks(state, black_king_pos);
        b_controlled_squares |= attacks;
        king_mobility -= bb_popcnt(attacks & ~all_pieces);
        king_shelter -= bb_popcnt(attacks & black_pieces);

        edge_pieces -= edge_table[black_king_pos];
        soldier_center -= center_table[black_king_pos];
    }

    int control = bb_popcnt(w_controlled_squares) - bb_popcnt(b_controlled_squares);

    int soldier_material = 0;
    int general_material = 0;
    int king_material = 0;

    // Only take material into account when the setup is over
    if (position->ply >= 10) {
        soldier_material = num_white_soldier - num_black_soldier;
        general_material = num_white_general - num_black_general;
        king_material = num_white_king - num_black_king;
    }

    EvalWeights w = state->weights;

    int opening_score = (
        soldier_material * 100 + // reference unit
        king_material * 10000 + // assert there is a king or continue to quiesce
        general_material * w.op_general_material +
        soldier_mobility * w.op_soldier_mobility +
        soldier_center * w.op_soldier_center +
        soldier_king_dist * w.op_soldier_king_dist +
        general_mobility * w.op_general_mobility +
        king_mobility * w.op_king_mobility +
        king_shelter * w.op_king_shelter +
        king_center * w.op_king_center +
        king_threats * w.op_king_threats +
        ss_pairs * w.op_ss_pair +
        sg_pairs * w.op_sg_pair +
        square_structures * w.op_square_structures +
        edge_pieces * w.op_edge_pieces +
        control * w.op_control
    );

    if (position->ply < 10) {
        return opening_score * turn;
    }

    int endgame_score = (
        king_material * 10000 + // assert there is a king or continue to quiesce
        soldier_material * w.eg_soldier_material + // reference unit
        general_material * w.eg_general_material +
        soldier_mobility * w.eg_soldier_mobility +
        soldier_center * w.eg_soldier_center +
        soldier_king_dist * w.eg_soldier_king_dist +
        general_mobility * w.eg_general_mobility +
        king_mobility * w.eg_king_mobility +
        king_shelter * w.eg_king_shelter +
        king_center * w.eg_king_center +
        king_threats * w.eg_king_threats +
        king_chase * w.eg_king_chase +
        ss_pairs * w.eg_ss_pair +
        sg_pairs * w.eg_sg_pair +
        square_structures * w.eg_square_structures +
        edge_pieces * w.eg_edge_pieces +
        control * w.eg_control
    );

    // PHASE CALCULATION

    int soldier_phase = 1;
    int general_phase = 4;

    int total_phase = 24 * soldier_phase + 6 * general_phase;

    int phase = total_phase;
    phase -= (num_white_soldier + num_black_soldier) * soldier_phase;
    phase -= (num_white_general + num_black_general) * general_phase;

    phase = (phase * 256 + (total_phase / 2)) / total_phase;

    return turn * (opening_score * (256 - phase) + endgame_score * phase) / 256;
}
