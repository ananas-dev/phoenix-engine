#include "search.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "tt.h"

#define INF 100000

double get_elapsed_time(State *state) {
    struct timeval now;
    gettimeofday(&now, NULL);

    // Calculate time difference in seconds with microsecond precision
    return (double) (now.tv_sec - state->start_time.tv_sec) +
           (double) (now.tv_usec - state->start_time.tv_usec) / 1000000.0;
}

// Sort order:
// 1. TT move
// 2. PV move
// 3. Killer move
// 4. History
int weight_move(State *state, Move move, Color side_to_move) {
    if (state->tt_best_move_sorting && move.from == state->tt_best_move_from && move.to == state->tt_best_move_to) {
        state->tt_best_move_sorting = false;
        return 69000;
    }

    if (state->pv_sorting) {
        Move pv_move = state->pv_table[0].moves[state->search_ply];
        if (pv_move.from == move.from && pv_move.to == move.to && pv_move.captures == move.captures) {
            state->pv_sorting = false;
            return 42000;
        }
    }

    // If move is not a capture we can use killer moves and history moves to improve
    if (bb_is_empty(move.captures)) {
        Move killer_move_1 = state->killer_moves[0][state->search_ply];
        Move killer_move_2 = state->killer_moves[1][state->search_ply];

        if (killer_move_1.from == move.from && killer_move_1.to == move.to) {
            return 6900;
        }

        if (killer_move_2.from == move.from && killer_move_2.to == move.to) {
            return 4200;
        }

        return state->history[side_to_move][move.from][move.to];
    }

    return 0;
}

// Here we use insertion sort since the move list is small
void sort_move_list(State *state, MoveList *move_list, Color side_to_move) {
    state->tt_best_move_sorting = (state->tt_best_move_flag != FLAG_NULL);

    int weights[move_list->size];

    for (int i = 0; i < move_list->size; i++) {
        weights[i] = weight_move(state, move_list->moves[i], side_to_move);
    }

    for (int i = 1; i < move_list->size; i++) {
        Move move = move_list->moves[i];
        int weight = weights[i];
        int j = i - 1;

        while (j >= 0 && weights[j] < weight) {
            move_list->moves[j + 1] = move_list->moves[j];
            weights[j + 1] = weights[j];
            j--;
        }

        move_list->moves[j + 1] = move;
        weights[j + 1] = weight;
    }
}

void update_clock(State *state) {
    double elapsed = get_elapsed_time(state);

    // Use 95% of allocated time to ensure we return before timeout
    if (elapsed >= state->max_time * 0.99) {
        state->time_over = true;
    }
}

int alpha_beta(State *state, Position *position, int depth, int alpha, int beta);

Move search(State *state, Position *position, double max_time_seconds) {
    state->search_ply = 0;

    memset(state->pv_table, 0, sizeof(state->pv_table));
    memset(state->killer_moves, 0, sizeof(state->killer_moves));
    memset(state->history, 0, sizeof(state->history));

    state->follow_pv = false;
    state->pv_sorting = false;

    state->time_over = false;
    state->nodes_visited = 0;
    gettimeofday(&state->start_time, NULL);
    state->max_time = max_time_seconds;

    int alpha = -INF;
    int beta = INF;

    for (int depth = 1; depth <= 100; depth++) {
        if (state->time_over) {
            break;
        }

        state->follow_pv = true;

        int score = alpha_beta(state, position, depth, alpha, beta);

        if (state->time_over) {
            printf("Depth=%d, Score=/\n", depth);
        } else {
            printf("Depth=%d, Score=%.2f\n", depth, (float)score/100.0f);
        }

        for (int i = 0; i < state->pv_table[0].size; i++) {
            move_print(state->pv_table[0].moves[i]);
            printf(" ");
        }

        printf("\n");

        // Return early if mate is found
        if (score >= INF - MAX_PLY) {
            return state->pv_table[0].moves[0];
        }

    }

    return state->pv_table[0].moves[0];
}

int quiesce(State *state, Position *position, int alpha, int beta) {
    if ((state->nodes_visited & 2047) == 0) {
        update_clock(state);
    }

    if (position_is_game_over(position)) {
        return INF - state->search_ply;
    }

    int stand_pat = eval(state, position);
    if (stand_pat >= beta) {
        return beta;
    }

    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    MoveList move_list;
    move_list.size = 0;

    legal_moves(state, position, &move_list);
    state->tt_best_move_flag = FLAG_NULL;
    sort_move_list(state, &move_list, position->side_to_move);

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.moves[i];
        // Check for non capture moves
        if (bb_is_empty(move.captures)) {
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

        state->search_ply++;

        Position new_position = make_move(state, position, move);

        int score = -quiesce(state, &new_position, -beta, -alpha);

        state->search_ply--;

        if (state->time_over) {
            return 0;
        }

        if (score > alpha) {
            alpha = score;

            if (score >= beta) {
                return beta;
            }
        }
    }

    return alpha;
}

int alpha_beta(State *state, Position *position, int depth, int alpha, int beta) {
    state->pv_table[state->search_ply].size = state->search_ply;

    if (position_is_game_over(position)) {
        return INF - state->search_ply;
    }

    // Update clock from time to time
    if ((state->nodes_visited & 2047) == 0) {
        update_clock(state);
    }

    PackedMove tt_best_move = NULL_PACKED_MOVE;
    int tt_value = tt_get(state, position, depth, alpha, beta, &tt_best_move);
    packed_move_extract(tt_best_move, &state->tt_best_move_flag, &state->tt_best_move_from, &state->tt_best_move_to);

    int pv_node = beta - alpha > 1;

    if (state->search_ply > 0 && tt_value != TT_MISS && pv_node == 0) {
        return tt_value;
    }

    if (depth == 0) {
        return quiesce(state, position, alpha, beta);
    }

    state->nodes_visited++;

    MoveList move_list;
    move_list.size = 0;
    legal_moves(state, position, &move_list);

    if (state->follow_pv) {
        state->follow_pv = false;

        // Iterate over the move list to see if one is part of the PV
        for (int i = 0; i < move_list.size; i++) {
            Move move = move_list.moves[i];
            Move pv_move = state->pv_table[0].moves[state->search_ply];
            if (pv_move.from == move.from && pv_move.to == move.to && pv_move.captures == move.captures) {
                state->pv_sorting = true;
                state->follow_pv = true;
            }
        }
    }

    sort_move_list(state, &move_list, position->side_to_move);

    Move best_move = {0};
    bool best_move_valid = false;

    EntryType tt_entry_type = ENTRY_TYPE_ALPHA;
    bool searched_first_move = false;

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.moves[i];
        Position new_position = make_move(state, position, move);

        state->search_ply++;

        int score;

        if (!searched_first_move) {
            score = -alpha_beta(state, &new_position, depth - 1, -beta, -alpha);
            searched_first_move = true;
        } else {
            score = -alpha_beta(state, &new_position, depth - 1, -alpha - 1, -alpha);

            if (score > alpha && score < beta) {
                score = -alpha_beta(state, &new_position, depth - 1, -beta, -alpha);
            }
        }

        state->search_ply--;

        if (state->time_over) {
            return 0;
        }

        if (score > alpha) {
            tt_entry_type = ENTRY_TYPE_EXACT;
            alpha = score;
            state->pv_table[state->search_ply].moves[state->search_ply] = move;

            best_move = move;
            best_move_valid = true;

            for (int next_ply = state->search_ply + 1; next_ply < state->pv_table[state->search_ply + 1].size; next_ply++) {
                state->pv_table[state->search_ply].moves[next_ply] = state->pv_table[state->search_ply + 1].moves[next_ply];
            }

            state->pv_table[state->search_ply].size = state->pv_table[state->search_ply + 1].size;

            if (score >= beta) {
                PackedMove best_move_packed = packed_move_new(FLAG_NORMAL, move.from, move.to);
                tt_set(state, position, depth, beta, ENTRY_TYPE_BETA, best_move_packed);

                if (bb_is_empty(move.captures)) {
                    state->killer_moves[1][state->search_ply] = state->killer_moves[0][state->search_ply];
                    state->killer_moves[0][state->search_ply] = move;
                    // Taken from https://www.chessprogramming.org/History_Heuristic
                    int clampedBonus = depth * depth;
                    state->history[position->side_to_move][move.from][move.to]
                        += clampedBonus - state->history[position->side_to_move][move.from][move.to] * clampedBonus / MAX_HISTORY;
                }


                return beta;
            }

        }
    }

    if (best_move_valid) {
        PackedMove best_move_packed = packed_move_new(FLAG_NORMAL, best_move.from, best_move.to);
        tt_set(state, position, depth, alpha, tt_entry_type, best_move_packed);
    } else {
        tt_set(state, position, depth, alpha, tt_entry_type, NULL_PACKED_MOVE);
    }


    return alpha;
}
