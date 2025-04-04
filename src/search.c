#include "search.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "tt.h"

#define INF 100000

int nodes_visited = 0;
struct timeval start_time;
double max_time;
bool time_over;
int search_depth;

MoveList pv_table = {0};

double get_elapsed_time() {
    struct timeval now;
    gettimeofday(&now, NULL);

    // Calculate time difference in seconds with microsecond precision
    return (double)(now.tv_sec - start_time.tv_sec) +
           (double)(now.tv_usec - start_time.tv_usec) / 1000000.0;
}

int compare_moves(const void *a, const void *b) {
    const Move *fst = a;
    const Move *snd = b;

    int fst_captures = bb_popcnt(fst->captures);
    int snd_captures = bb_popcnt(snd->captures);

    if (fst_captures != 0 || snd_captures != 0) {
        return fst_captures - snd_captures;
    }

    return 1;
}

void sort_move_list(MoveList *move_list) {
    qsort(move_list->moves, move_list->size, sizeof(Move), compare_moves);
}

bool is_time_up(void) {
    // Check time every 1000 nodes or so to avoid expensive clock() calls
    double elapsed = get_elapsed_time();


    // Use 95% of allocated time to ensure we return before timeout
    if (elapsed >= max_time * 0.99) {
        time_over = true;
        return true;
    }
    return false;
}

Move search(Position *position, double max_time_seconds) {
    time_over = false;
    nodes_visited = 0;
    gettimeofday(&start_time, NULL);
    max_time = max_time_seconds;

    Move last_completed_best_move = {};

    for (int depth = 1; depth <= 100; depth++) {
        if (time_over) {
            break;
        }

        MoveList move_list;
        move_list.size = 0;
        legal_moves(position, &move_list);
        sort_move_list(&move_list);

        int best_score = -INF;
        Move current_best_move = {};
        bool depth_completed = true;

        for (int i = 0; i < move_list.size; i++) {
            Move move = move_list.moves[i];
            Position new_position = make_move(position, move);

            int score = -alpha_beta(&new_position, depth - 1, -INF, INF);

            if (time_over) {
                depth_completed = false;
                break;
            }

            if (score > best_score) {
                best_score = score;
                current_best_move = move;
            }
        }

        if (depth_completed) {
            last_completed_best_move = current_best_move;
            printf("Depth %d: Best move eval = %d\n", depth, best_score);
        } else {
            printf("Depth %d: Search incomplete (time limit)\n", depth);
            break;
        }
    }

    return last_completed_best_move;
}

int quiesce(Position *position, int alpha, int beta) {
    if ((nodes_visited & 2047) == 0) {
        if (is_time_up()) {
            return 0;
        }
    }

    if (position_is_game_over(position)) {
        return -INF + position->ply;
    }

    int stand_pat = eval(position);
    int best_value = stand_pat;
    if (stand_pat >= beta)
        return stand_pat;
    if (alpha < stand_pat)
        alpha = stand_pat;

    MoveList move_list;
    move_list.size = 0;
    legal_moves(position, &move_list);
    sort_move_list(&move_list);

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
    if (position_is_game_over(position)) {
        return -INF + position->ply;
    }

    // Check every 1000 nodes
    if ((nodes_visited & 2047) == 0) {
        if (is_time_up()) {
            return 0;
        }
    }

    int tt_value = tt_get(position, depth, alpha, beta);
    if (tt_value != TT_MISS) {
        return tt_value;
    }

    if (depth == 0) {
        return quiesce(position, alpha, beta);
    }

    nodes_visited++;

    MoveList move_list;
    move_list.size = 0;
    legal_moves(position, &move_list);
    sort_move_list(&move_list);

    if (move_list.size == 0) {
        return -INF + position->ply;
    }

    int best_value = -INF;
    Move best_move = {0};

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.moves[i];
        Position new_position = make_move(position, move);
        int score = -alpha_beta(&new_position, depth - 1, -beta, -alpha);

        if (score > best_value) {
            best_value = score;
            best_move = move;
            if (score > alpha) {
                alpha = score;
            }
        }

        if (score >= beta) {
            tt_set(position, depth, best_value, ENTRY_TYPE_BETA);
            return best_value;
        }
    }

    EntryType type;
    if (best_value <= alpha) {
        type = ENTRY_TYPE_ALPHA; // Upper bound (fail-low)
    } else {
        type = ENTRY_TYPE_EXACT; // Exact score
    }

    tt_set(position, depth, best_value, type);

    return best_value;
}
