#include "eval.h"

#include <stdlib.h>
#include <string.h>

#include "bitboard.h"
#include "movegen.h"

void square_structure_init(Context *ctx) {
    for (File file = FILE_A; file <= FILE_G; file++) {
        for (Rank rank = RANK_1; rank <= RANK_6; rank++) {
            Bitboard square_structure = BB_EMPTY;
            square_structure |= bb_from_sq(sq_get(file, rank));
            square_structure |= bb_from_sq(sq_get(file + 1, rank));
            square_structure |= bb_from_sq(sq_get(file + 1, rank + 1));
            square_structure |= bb_from_sq(sq_get(file, rank + 1));

            ctx->square_structure_table[file][rank] = square_structure;
        }
    }
}

int count_square_structures(Context *ctx, Position *position) {
    int count = 0;

    for (File file = FILE_A; file <= FILE_G; file++) {
        for (Rank rank = RANK_1; rank <= RANK_6; rank++) {
            Bitboard square_structure = ctx->square_structure_table[file][rank];

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

void eval_init(Context *ctx) {
    memset(ctx->weights, 0, sizeof(int) * W_COUNT);
    ctx->weights[W_OP_GENERAL_MATERIAL] = 500;
    ctx->weights[W_EG_GENERAL_MATERIAL] = 500;
    ctx->weights[W_OP_SOLDIER_MATERIAL] = 100;
    ctx->weights[W_EG_SOLDIER_MATERIAL] = 100;
    ctx->weights[W_OP_GENERAL_MOBILITY] = 8;
    ctx->weights[W_EG_GENERAL_MOBILITY] = 8;
    ctx->weights[W_OP_SOLDIER_KING_DIST] = 5;
    ctx->weights[W_EG_SOLDIER_KING_DIST] = 5;
    ctx->weights[W_EG_KING_CHASE] = 10;

    for (Square i = SQ_A1; i <= SQ_H7; i++) {
        for (Square j = SQ_A1; j <= SQ_H7; j++) {
            ctx->distance[i][j] = 13 - (abs(sq_file(i) - sq_file(j)) + abs(sq_rank(i) - sq_rank(j)));
        }
    }

    square_structure_init(ctx);
}

int eval(Context *ctx, Position *position) {
    int turn = position->side_to_move == COLOR_WHITE ? 1 : -1;
    int *w = ctx->weights;

    int num_white_soldier = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
    int num_white_general = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_GENERAL]);
    int num_white_king = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_KING]);
    int num_black_soldier = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
    int num_black_general = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_GENERAL]);
    int num_black_king = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_KING]);

    bool both_kings_present = num_white_king > 0 && num_black_king > 0;

    // K(S) vs K(S) draw
    if (
        num_white_soldier <= 1 &&
        num_black_soldier <= 1 &&
        num_white_general == 0 &&
        num_black_general == 0 &&
        both_kings_present
    ) {
        return 0;
    }

    // KG vs KS draw
    bool one_vs_one_soldier_general =
            (num_white_soldier == 1 && num_black_soldier == 0 &&
             num_white_general == 0 && num_black_general == 1) ||
            (num_white_soldier == 0 && num_black_soldier == 1 &&
             num_white_general == 1 && num_black_general == 0);

    if (one_vs_one_soldier_general && both_kings_present) {
        return 0;
    }

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

    int square_structures = count_square_structures(ctx, position);

    int op_soldier_position = 0;
    int op_general_position = 0;
    int op_king_position = 0;
    int eg_soldier_position = 0;
    int eg_general_position = 0;
    int eg_king_position = 0;

    int ss_pairs = 0;
    int sg_pairs = 0;

    int king_threats = 0;

    int soldier_mobility = 0;
    int soldier_king_dist = 0;

    Bitboard w_controlled_squares = BB_EMPTY;
    Bitboard b_controlled_squares = BB_EMPTY;

    // SOLDIER ITER

    Bitboard w_soldier_it = position->pieces[COLOR_WHITE][PIECE_SOLDIER] & BB_USED;
    while (w_soldier_it) {
        Square from = bb_it_next(&w_soldier_it);
        Bitboard attacks = soldier_attacks(ctx, from);
        w_controlled_squares |= attacks;

        ss_pairs += bb_popcnt(attacks & position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
        sg_pairs += bb_popcnt(attacks & position->pieces[COLOR_WHITE][PIECE_GENERAL]);
        soldier_mobility += bb_popcnt(attacks & ~all_pieces);

        if (num_black_king > 0) {
            soldier_king_dist += ctx->distance[from][black_king_pos];
        }

        king_threats += bb_popcnt(attacks & position->pieces[COLOR_BLACK][PIECE_KING]);

        op_soldier_position += w[W_OP_SOLDIER_PIECE_TABLE + from];
        eg_soldier_position += w[W_EG_SOLDIER_PIECE_TABLE + from];
    }

    Bitboard b_soldier_it = position->pieces[COLOR_BLACK][PIECE_SOLDIER] & BB_USED;
    while (b_soldier_it) {
        Square from = bb_it_next(&b_soldier_it);
        Bitboard attacks = soldier_attacks(ctx, from);
        b_controlled_squares |= attacks;

        ss_pairs -= bb_popcnt(attacks & position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
        sg_pairs -= bb_popcnt(attacks & position->pieces[COLOR_BLACK][PIECE_GENERAL]);
        soldier_mobility -= bb_popcnt(attacks & ~all_pieces);

        if (num_white_king > 0) {
            soldier_king_dist -= ctx->distance[from][white_king_pos];
        }

        king_threats -= bb_popcnt(attacks & position->pieces[COLOR_WHITE][PIECE_KING]);

        op_soldier_position -= w[W_OP_SOLDIER_PIECE_TABLE + sq_mirror(from)];
        eg_soldier_position -= w[W_EG_SOLDIER_PIECE_TABLE + sq_mirror(from)];
    }

    // GENERAL ITER

    int general_mobility = 0;

    Bitboard w_general_it = position->pieces[COLOR_WHITE][PIECE_GENERAL] & BB_USED;
    while (w_general_it) {
        Square from = bb_it_next(&w_general_it);
        Bitboard attacks = general_attacks(ctx, from, all_pieces);
        w_controlled_squares |= attacks;
        general_mobility += bb_popcnt(attacks & ~all_pieces);

        king_threats += bb_popcnt(attacks & position->pieces[COLOR_BLACK][PIECE_KING]);

        op_general_position += w[W_OP_GENERAL_PIECE_TABLE + from];
        eg_general_position += w[W_EG_GENERAL_PIECE_TABLE + from];
    }

    Bitboard b_general_it = position->pieces[COLOR_BLACK][PIECE_GENERAL] & BB_USED;
    while (b_general_it) {
        Square from = bb_it_next(&b_general_it);
        Bitboard attacks = general_attacks(ctx, from, all_pieces);
        b_controlled_squares |= attacks;
        general_mobility -= bb_popcnt(attacks & ~all_pieces);

        king_threats -= bb_popcnt(attacks & position->pieces[COLOR_WHITE][PIECE_KING]);

        op_general_position -= w[W_OP_GENERAL_PIECE_TABLE + sq_mirror(from)];
        eg_general_position -= w[W_EG_GENERAL_PIECE_TABLE + sq_mirror(from)];
    }

    // KING ITER

    int king_mobility = 0;
    int king_shelter = 0;
    int king_chase = 0;

    if (num_white_king > 0 && num_black_king > 0) {
        int kings_distance = ctx->distance[white_king_pos][black_king_pos];
        if (num_black_general == 0 && num_white_general > 0) {
            king_chase += kings_distance;
        } else if (num_white_general == 0 && num_black_general > 0) {
            king_chase -= kings_distance;
        }
    }


    if (num_white_king > 0) {
        Bitboard attacks = king_attacks(ctx, white_king_pos);
        w_controlled_squares |= attacks;
        king_mobility += bb_popcnt(attacks & ~all_pieces);
        king_shelter += bb_popcnt(attacks & white_pieces);

        op_king_position += w[W_OP_KING_PIECE_TABLE + white_king_pos];
        eg_king_position += w[W_EG_KING_PIECE_TABLE + white_king_pos];
    }

    if (num_black_king > 0) {
        Bitboard attacks = king_attacks(ctx, black_king_pos);
        b_controlled_squares |= attacks;
        king_mobility -= bb_popcnt(attacks & ~all_pieces);
        king_shelter -= bb_popcnt(attacks & black_pieces);

        op_king_position -= w[W_OP_KING_PIECE_TABLE + sq_mirror(black_king_pos)];
        eg_king_position -= w[W_EG_KING_PIECE_TABLE + sq_mirror(black_king_pos)];
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

    int opening_score = (
        king_material * 10000 + // assert there is a king or continue to quiesce
        soldier_material * w[W_OP_SOLDIER_MATERIAL] +
        general_material * w[W_OP_GENERAL_MATERIAL] +
        soldier_mobility * w[W_OP_SOLDIER_MOBILITY] +
        soldier_king_dist * w[W_OP_SOLDIER_KING_DIST] +
        general_mobility * w[W_OP_GENERAL_MOBILITY] +
        king_mobility * w[W_OP_KING_MOBILITY] +
        king_shelter * w[W_OP_KING_SHELTER] +
        king_threats * w[W_OP_KING_THREATS] +
        ss_pairs * w[W_OP_SS_PAIR] +
        sg_pairs * w[W_OP_SG_PAIR] +
        square_structures * w[W_OP_SQUARE_STRUCTURES] +
        control * w[W_OP_CONTROL] +
        op_soldier_position +
        op_general_position +
        op_king_position
    );

    if (position->ply < 10) {
        return opening_score * turn;
    }

    int endgame_score = (
        king_material * 10000 + // assert there is a king or continue to quiesce
        soldier_material * w[W_EG_SOLDIER_MATERIAL] +
        general_material * w[W_EG_GENERAL_MATERIAL] +
        soldier_mobility * w[W_EG_SOLDIER_MOBILITY] +
        soldier_king_dist * w[W_EG_SOLDIER_KING_DIST] +
        general_mobility * w[W_EG_GENERAL_MOBILITY] +
        king_mobility * w[W_EG_KING_MOBILITY] +
        king_shelter * w[W_EG_KING_SHELTER] +
        king_threats * w[W_EG_KING_THREATS] +
        king_chase * w[W_EG_KING_CHASE] +
        ss_pairs * w[W_EG_SS_PAIR] +
        sg_pairs * w[W_EG_SG_PAIR] +
        square_structures * w[W_EG_SQUARE_STRUCTURES] +
        control * w[W_EG_CONTROL] +
        eg_soldier_position +
        eg_general_position +
        eg_king_position
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
