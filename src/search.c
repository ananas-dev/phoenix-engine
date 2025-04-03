#include "search.h"

#include <stdio.h>

#include "eval.h"
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
