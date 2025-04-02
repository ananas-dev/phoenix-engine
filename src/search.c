#include "search.h"

#include <stdio.h>

#include "movegen.h"
#include "position.h"

#define INF 10000

Move search(Position *position, int depth) {
    MoveList move_list = {0};
    legal_moves(position, &move_list);

    int best_score = -INF;
    Move best_move = {};

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.moves[i];
        Position new_position = make_move(position, move);

        int score = -alpha_beta(&new_position, depth - 1, -INF, INF);

        if (score >= best_score) {
            best_move = move;
            best_score = score;
        }
    }

    printf("Best move eval: %d\n", best_score);

    return best_move;
}

static int eval(Position *position) {
    int turn = position->side_to_move == COLOR_WHITE ? 1 : -1;

    int num_white_solider = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
    int num_white_general = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_GENERAL]);
    int num_white_king = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_KING]);
    int num_black_solider = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
    int num_black_general = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_GENERAL]);
    int num_black_king = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_KING]);

    int material_score = + 1000 * (num_white_king - num_black_king)
                         + 500 * (num_white_general - num_black_general)
                         + 100 * (num_white_solider - num_black_solider);

    MoveList move_list = {0};
    legal_moves(position, &move_list);
    int num_white_legal_move = move_list.size;

    Position black_pov = make_null_move(position);
    move_list.size = 0;
    legal_moves(&black_pov, &move_list);
    int num_black_legal_move = move_list.size;

    int mobility_score = 10 * (num_white_legal_move - num_black_legal_move);

    return (material_score + mobility_score) * turn;
};

int alpha_beta(Position *position, int depth, int alpha, int beta) {
    if (position_is_game_over(position)) {
        return -INF;
    }

    if (depth == 0) {
        return eval(position);
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
            return best_value;
        }

    }

    return best_value;
}
