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
    int weights[] = {69, 189, 138, 227, 197, 210, 242, 185, 147, 121, 134, 192, 217, 199, 221, 249, 199, 136, 113, 102, 187, 188, 249, 501, 135, 140, 133, 91, 125, 45, 211, 250, 200, 130, 184, 144, 113, 73, 124, 264, 180, 131, 127, 164, 124, 112, 195, 217, 175, 169, 196, 156, 183, 106, 135, 44, 67, -22, 55, 68, -164, 11, 0, 135, -78, -152, -100, -105, -174, -34, -25, 18, 10, -129, -147, -198, -140, -117, -63, -84, 47, -10, -198, -220, -203, -127, -106, -194, 160, 54, -26, -190, -148, -123, -153, 76, 171, 105, 37, -40, -166, -190, -97, 77, 200, 199, 148, 73, 21, -76, -144, 26, 11, -123, -199, -110, -150, 198, -199, -199, -321, -191, -196, -198, -196, -198, -180, -199, -114, -239, -433, -174, -188, 177, -198, 211, -42, -126, -202, -187, 181, -194, -186, 66, 72, -34, -99, -213, -234, -196, -200, -153, 116, 50, -44, -136, -202, -208, -193, -181, 190, 118, 68, -46, -168, -372, -426, -180, 106, 61, 73, 54, 67, 83, 25, 26, 99, 99, 52, 50, 61, 99, -2, -11, 75, 82, 68, 92, 82, 99, 68, 15, 95, 84, 92, 94, 99, 108, 73, 54, 74, 96, 68, 75, 104, 112, 64, 34, 96, 93, 102, 78, 98, 106, 47, 31, 89, 91, 98, 93, 52, 83, 52, 105, 201, 163, 162, 136, 205, 201, 204, 227, 191, 206, 181, 191, 197, 198, 115, 227, 170, 189, 208, 199, 202, 162, 202, 215, 117, 135, 202, 191, 181, 181, 157, 229, 94, 100, 168, 217, 211, 187, 198, 154, 99, 103, 141, 164, 212, 223, 199, 138, 123, 73, 96, 114, 138, 196, 198, 209, -23, -3, 9, 79, 116, 105, 10, -19, 38, 35, 54, 103, 83, 101, 104, 47, -16, 72, 58, 48, 49, -15, 116, 54, -6, 47, 9, -35, -97, 2, 105, 56, -18, 25, 31, 59, 20, 24, 129, 63, -20, 22, 45, 57, 102, 105, 78, 29, -15, -9, -22, 29, -9, 53, 30, 19, 42, 464, 14, 8, 12, -24, 34, 123, -4, 26, 8, 8, 13, 506, -1, 8, 5, 4, 13, -11, -16, 7, 23, -47, -8};

    for (int i = 0; i < W_COUNT; i++) {
        ctx->weights[i] = weights[i];
    }

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
