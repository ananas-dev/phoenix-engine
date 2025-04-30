#include "search.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdatomic.h>

#include "movegen.h"
#include "position.h"
#include "tt.h"

#define INF 100000

int64_t get_elapsed_time(State *state) {
    struct timeval now;
    gettimeofday(&now, NULL);

    int64_t seconds = now.tv_sec - state->start_time.tv_sec;
    int64_t useconds = now.tv_usec - state->start_time.tv_usec;

    return (seconds * 1000) + (useconds / 1000);
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
        Move pv_move = state->pv_table[0].elems[state->search_ply];
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
        weights[i] = weight_move(state, move_list->elems[i], side_to_move);
    }

    for (int i = 1; i < move_list->size; i++) {
        Move move = move_list->elems[i];
        int weight = weights[i];
        int j = i - 1;

        while (j >= 0 && weights[j] < weight) {
            move_list->elems[j + 1] = move_list->elems[j];
            weights[j + 1] = weights[j];
            j--;
        }

        move_list->elems[j + 1] = move;
        weights[j + 1] = weight;
    }
}

void update_clock(State *state) {
    int64_t elapsed = get_elapsed_time(state);

    if (elapsed + 10 >= state->max_time) {
        state->time_over = true;
    }
}

bool is_repetition(State *state, Position *position) {
    if (state->game_history.size == 0 || state->search_ply == 0) {
        return false;
    }

    for (int i = state->game_history.size - 1; i >= 0; i -= 1) {
        if (state->game_history.elems[i] == position->hash) {
            return true;
        }
    }

    return false;
}

int alpha_beta(State *state, Position *position, int depth, int alpha, int beta);

void search(State *state) {
    state->search_ply = 0;

    memset(state->pv_table, 0, sizeof(state->pv_table));
    memset(state->killer_moves, 0, sizeof(state->killer_moves));
    memset(state->history, 0, sizeof(state->history));

    state->follow_pv = false;
    state->pv_sorting = false;

    state->time_over = false;
    state->nodes_visited = 0;
    gettimeofday(&state->start_time, NULL);

    int alpha = -INF;
    int beta = INF;

    Move best_move = {0};
    bool found_mate = false;

    for (int depth = 1; depth <= 100; depth++) {
        if (state->time_over || atomic_load(&state->stopped)) {
            break;
        }

        // Only search on full turns during the setup phase
        if (state->position.ply + depth < 10 && depth % 2 == 1 - (int) state->position.side_to_move) {
            continue;
        }

        state->tt_best_move_flag = FLAG_NULL;
        state->follow_pv = true;

        int score = alpha_beta(state, &state->position, depth, alpha, beta);

        if (state->debug) {
            // if (state->time_over || atomic_load(&state->stopped)) {
            //     printf("Depth=%d, Score=/\n", depth);
            // } else {
            //     printf("Depth=%d, Score=%.2f\n", depth, (float) score / 100.0f);
            // }
            //
            // for (int i = 0; i < state->pv_table[0].size; i++) {
            //     move_print(state->pv_table[0].elems[i]);
            //     printf(" ");
            // }
            //
            // printf("\n");
        }

        if (state->pv_table[0].size > 0) {
            best_move = state->pv_table[0].elems[0];
        }

        if (score >= INF - MAX_PLY || score <= -INF + MAX_PLY) {
            found_mate = true;
            if (score > 0) {
                // Only break if winning
                break;
            }
        }
    }

    // FIXME: Thread safety
    printf("bestmove %s\n", move_to_uci(best_move));
    fflush(stdout);
}

int quiesce(State *state, Position *position, int alpha, int beta) {
    Color turn = position->side_to_move;

    if ((state->nodes_visited & 2047) == 0) {
        update_clock(state);
    }

    GameState game_state = position_state(position);
    if (game_state == STATE_WIN) {
        return INF - state->search_ply;
    }
    if (game_state == STATE_LOSS) {
        return -(INF - state->search_ply);
    }
    if (game_state == STATE_DRAW || is_repetition(state, position)) {
        return 0;
    }

    int stand_pat = network_evaluate(&state->net, &position->accumulators[turn], &position->accumulators[1 - turn]);
    if (stand_pat >= beta) {
        return beta;
    }

    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    MoveList move_list;
    move_list.size = 0;

    legal_moves(position, &move_list);
    state->tt_best_move_flag = FLAG_NULL;
    sort_move_list(state, &move_list, position->side_to_move);

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.elems[i];
        // Check for non capture moves
        if (bb_is_empty(move.captures)) {
            // Skip stack check in setup phase
            Bitboard to_mask = bb_from_sq(move.to);
            Bitboard new_general = to_mask & position->pieces[position->side_to_move][PIECE_SOLDIER];
            Bitboard new_king = to_mask & position->pieces[position->side_to_move][PIECE_GENERAL];

            // Skip move if its not a stack
            if (bb_is_empty(new_general) && bb_is_empty(new_king)) {
                continue;
            }
        }

        state->search_ply++;

        Position new_position = make_move(position, &state->net, move);

        int score = -quiesce(state, &new_position, -beta, -alpha);

        state->search_ply--;

        if (state->time_over || atomic_load(&state->stopped)) {
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

    GameState game_state = position_state(position);
    if (game_state == STATE_WIN) {
        return INF - state->search_ply;
    }
    if (game_state == STATE_LOSS) {
        return -(INF - state->search_ply);
    }
    if (game_state == STATE_DRAW || is_repetition(state, position)) {
        return 0;
    }

    // Update clock from time to time
    if ((state->nodes_visited & 2047) == 0) {
        update_clock(state);
    }

    PackedMove tt_best_move = NULL_PACKED_MOVE;
    int tt_value = tt_get(&state->tt, position, depth, alpha, beta, &tt_best_move);
    packed_move_extract(tt_best_move, &state->tt_best_move_flag, &state->tt_best_move_from, &state->tt_best_move_to);

    int pv_node = beta - alpha > 1;

    if (state->search_ply > 0 && tt_value != TT_MISS && (pv_node == 0 || tt_value == 0)) {
        return tt_value;
    }

    if (depth == 0) {
        // Only search at even depth during the setup
        if (position->ply < 10) {
            return network_evaluate(&state->net, &position->accumulators[position->side_to_move],
                                    &position->accumulators[1 - position->side_to_move]);
        }

        return quiesce(state, position, alpha, beta);
    }

    state->nodes_visited++;

    MoveList move_list;
    move_list.size = 0;
    legal_moves(position, &move_list);

    if (state->follow_pv) {
        state->follow_pv = false;

        // Iterate over the move list to see if one is part of the PV
        for (int i = 0; i < move_list.size; i++) {
            Move move = move_list.elems[i];
            Move pv_move = state->pv_table[0].elems[state->search_ply];
            if (pv_move.from == move.from && pv_move.to == move.to && pv_move.captures == move.captures) {
                state->pv_sorting = true;
                state->follow_pv = true;
            }
        }
    }

    sort_move_list(state, &move_list, position->side_to_move);

    Move best_move = {0};
    bool best_move_valid = false;

    TTEntryType tt_entry_type = ENTRY_TYPE_ALPHA;

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.elems[i];
        Position new_position = make_move(position, &state->net, move);

        state->search_ply++;

        int score;

        if (i == 0) {
            score = -alpha_beta(state, &new_position, depth - 1, -beta, -alpha);
        } else {
            int reduction = (
                                i >= 4 &&
                                depth >= 3 &&
                                bb_popcnt(move.captures) == 0 &&
                                !position->can_create_general &&
                                !position->can_create_king
                            )
                                ? 1
                                : 0;

            score = -alpha_beta(state, &new_position, depth - 1 - reduction, -alpha - 1, -alpha);

            if (score > alpha && score < beta) {
                score = -alpha_beta(state, &new_position, depth - 1, -beta, -alpha);
            }
        }

        state->search_ply--;

        if (state->time_over || atomic_load(&state->stopped)) {
            return 0;
        }

        if (score > alpha) {
            tt_entry_type = ENTRY_TYPE_EXACT;
            alpha = score;
            state->pv_table[state->search_ply].elems[state->search_ply] = move;

            best_move = move;
            best_move_valid = true;

            for (int next_ply = state->search_ply + 1; next_ply < state->pv_table[state->search_ply + 1].size; next_ply
                 ++) {
                state->pv_table[state->search_ply].elems[next_ply] = state->pv_table[state->search_ply + 1].elems[
                    next_ply];
            }

            state->pv_table[state->search_ply].size = state->pv_table[state->search_ply + 1].size;

            if (score >= beta) {
                PackedMove best_move_packed = packed_move_new(FLAG_NORMAL, move.from, move.to);
                tt_set(&state->tt, position, depth, beta, ENTRY_TYPE_BETA, best_move_packed);

                if (bb_is_empty(move.captures)) {
                    state->killer_moves[1][state->search_ply] = state->killer_moves[0][state->search_ply];
                    state->killer_moves[0][state->search_ply] = move;
                    // Taken from https://www.chessprogramming.org/History_Heuristic
                    int clampedBonus = depth * depth;
                    state->history[position->side_to_move][move.from][move.to]
                            += clampedBonus - state->history[position->side_to_move][move.from][move.to] * clampedBonus
                            / MAX_HISTORY;
                }


                return beta;
            }
        }
    }

    if (best_move_valid) {
        PackedMove best_move_packed = packed_move_new(FLAG_NORMAL, best_move.from, best_move.to);
        tt_set(&state->tt, position, depth, alpha, tt_entry_type, best_move_packed);
    } else {
        tt_set(&state->tt, position, depth, alpha, tt_entry_type, NULL_PACKED_MOVE);
    }


    return alpha;
}
