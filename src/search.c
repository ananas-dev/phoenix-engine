#include "search.h"

#include <stdio.h>

#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "tt.h"

#define INF 10000

Move search(Position *position, int max_depth) {
    Move best_move = {};

    for (int depth = 1; depth <= max_depth; depth++) {
        MoveList move_list = {0};
        legal_moves(position, &move_list);

        int best_score = -INF;

        for (int i = 0; i < move_list.size; i++) {
            Move move = move_list.moves[i];
            Position new_position = make_move(position, move);

            int score = -alpha_beta(&new_position, depth - 1, -INF, INF);

            if (score > best_score) {
                best_score = score;
                best_move = move;
            }
        }

        printf("Depth %d: Best move eval = %d\n", depth, best_score);
    }

    return best_move;
}

int quiesce(Position *position, int alpha, int beta) {
    if (position_is_game_over(position)) {
        return -INF + position->ply;
    }

    int stand_pat = eval(position);
    int best_value = stand_pat;
    if (stand_pat >= beta)
        return stand_pat;
    if (alpha < stand_pat)
        alpha = stand_pat;

    MoveList move_list = {0};
    legal_moves(position, &move_list);

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.moves[i];
        // Check for non capture moves
        if (bb_is_empty(move.captured)) {
            // Skip stack check in setup phase
            if (position->ply < 10) {
                continue;
            }

            Bitboard to_mask = bb_from_sq(move.to);
            Bitboard new_general = to_mask & position->pieces[position->side_to_move][PIECE_SOLDIER];
            Bitboard new_king = to_mask & position->pieces[position->side_to_move][PIECE_GENERAL];

            // Skip move if its not a stack
            if (bb_is_empty(new_general) && bb_is_empty(new_king)) {
                continue;
            }
        }

        Position new_position = make_move(position, move);

        int score = -quiesce(&new_position, -beta, -alpha);

        if (score >= beta)
            return score;
        if (score > best_value)
            best_value = score;
        if (score > alpha)
            alpha = score;
    }

    return best_value;
}

int alpha_beta(Position *position, int depth, int alpha, int beta) {
    int tt_value = tt_get(position, depth, alpha, beta);
    if (tt_value != TT_MISS) {
        return tt_value;
    }

    if (position_is_game_over(position)) {
        // Add ply to find shorter mates
        return -INF + position->ply;
    }

    if (depth == 0) {
        return quiesce(position, alpha, beta);
    }

    MoveList move_list = {0};
    legal_moves(position, &move_list);

    int best_value = -INF;

    for (int i = 0; i < move_list.size; i++) {
        Position new_position = make_move(position, move_list.moves[i]);
        int score = -alpha_beta(&new_position, depth - 1, -beta, -alpha);

        if (score > best_value) {
            best_value = score;
            if (score > alpha) {
                alpha = score;
            }
        }

        if (score >= beta) {
            tt_set(position, depth, beta, ENTRY_TYPE_BETA);
            return best_value;
        }
    }

    EntryType type;
    if (best_value <= alpha) {
        type = ENTRY_TYPE_ALPHA;  // Upper bound (fail-low)
    } else {
        type = ENTRY_TYPE_EXACT;  // Exact score
    }

    tt_set(position, depth, best_value, type);

    return best_value;
}
