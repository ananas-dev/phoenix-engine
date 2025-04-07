#include "search.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

PackedMoveFlag tt_best_move_flag;
Square tt_best_move_from;
Square tt_best_move_to;
bool tt_best_move_sorting;

#define MAX_PLY 128
#define PV_TABLE_SIZE 64

typedef struct {
    Move moves[PV_TABLE_SIZE];
    int size;
} PVLine;

PVLine pv_table[MAX_PLY];
bool pv_sorting = true;
bool follow_pv = true;

int search_ply = 0;

double get_elapsed_time() {
    struct timeval now;
    gettimeofday(&now, NULL);

    // Calculate time difference in seconds with microsecond precision
    return (double) (now.tv_sec - start_time.tv_sec) +
           (double) (now.tv_usec - start_time.tv_usec) / 1000000.0;
}

// Sort order:
// 1. TT move
// 2. PV move
// 3. Normal move
int weight_move(Move move) {
    if (tt_best_move_sorting && move.from == tt_best_move_from && move.to == tt_best_move_to) {
        tt_best_move_sorting = false;
        return 2;
    }

    if (pv_sorting) {
        Move pv_move = pv_table[0].moves[search_ply];
        if (pv_move.from == move.from && pv_move.to == move.to && pv_move.captures == move.captures) {
            pv_sorting = false;
            return 1;
        }
    }

    return 0;
}

// Here we use insertion sort since the move list is small and almost sorted
void sort_move_list(MoveList *move_list) {
    tt_best_move_sorting = (tt_best_move_flag != FLAG_NULL);

    int weights[move_list->size];

    for (int i = 0; i < move_list->size; i++) {
        weights[i] = weight_move(move_list->moves[i]);
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

int alpha_beta(Position *position, int depth, int alpha, int beta);

Move search(Position *position, double max_time_seconds) {
    search_ply = 0;

    memset(pv_table, 0, sizeof(pv_table));

    follow_pv = false;
    pv_sorting = false;

    time_over = false;
    nodes_visited = 0;
    gettimeofday(&start_time, NULL);
    max_time = max_time_seconds;

    int alpha = -INF;
    int beta = INF;

    Move best_move = {0};

    for (int depth = 1; depth <= 100; depth++) {
        if (time_over) {
            break;
        }

        follow_pv = true;

        int score = alpha_beta(position, depth, alpha, beta);

        if (!time_over) {
            best_move = pv_table[0].moves[0];

            printf("Depth=%d, Score=%.2f\n", depth, (float)score/100.0f);

            for (int i = 0; i < pv_table[0].size; i++) {
                move_print(pv_table[0].moves[i]);
                printf(" ");
            }

            printf("\n");

            // Return early if mate is found
            if (score >= INF - MAX_PLY) {
                return best_move;
            }
        }

    }

    return best_move;
}

int quiesce(Position *position, int alpha, int beta) {
    if ((nodes_visited & 2047) == 0) {
        if (is_time_up()) {
            return 0;
        }
    }

    if (position_is_game_over(position)) {
        return INF - search_ply;
    }

    int stand_pat = eval(position);
    if (stand_pat >= beta) {
        return stand_pat;
    }

    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    MoveList move_list;
    move_list.size = 0;
    legal_moves(position, &move_list);
    tt_best_move_flag = FLAG_NULL;
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

        if (score > alpha) {
            alpha = score;

            if (score >= beta) {
                return beta;
            }
        }
    }

    return alpha;
}

int alpha_beta(Position *position, int depth, int alpha, int beta) {
    pv_table[search_ply].size = search_ply;

    if (position_is_game_over(position)) {
        return INF - search_ply;
    }

    // Check every 1000 nodes
    if ((nodes_visited & 2047) == 0) {
        if (is_time_up()) {
            return 0;
        }
    }

    PackedMove tt_best_move = NULL_PACKED_MOVE;
    int tt_value = tt_get(position, depth, alpha, beta, &tt_best_move);
    packed_move_extract(tt_best_move, &tt_best_move_flag, &tt_best_move_from, &tt_best_move_to);

    int pv_node = beta - alpha > 1;

    if (search_ply > 0 && tt_value != TT_MISS && pv_node == 0) {
        return tt_value;
    }

    if (depth == 0) {
        return quiesce(position, alpha, beta);
    }

    nodes_visited++;

    MoveList move_list;
    move_list.size = 0;
    legal_moves(position, &move_list);

    if (follow_pv) {
        follow_pv = false;

        // Iterate over the move list to see if one is part of the PV
        for (int i = 0; i < move_list.size; i++) {
            Move move = move_list.moves[i];
            Move pv_move = pv_table[0].moves[search_ply];
            if (pv_move.from == move.from && pv_move.to == move.to && pv_move.captures == move.captures) {
                pv_sorting = true;
                follow_pv = true;
            }
        }
    }

    sort_move_list(&move_list);

    if (follow_pv) {
        Move fst = move_list.moves[0];
        Move snd = pv_table[0].moves[search_ply];
        assert(fst.from == snd.from && fst.to == snd.to);
    }

    Move best_move = {0};

    EntryType tt_entry_type = ENTRY_TYPE_ALPHA;
    bool searched_first_move = false;

    for (int i = 0; i < move_list.size; i++) {
        Move move = move_list.moves[i];
        Position new_position = make_move(position, move);

        search_ply++;

        int score;

        if (!searched_first_move) {
            score = -alpha_beta(&new_position, depth - 1, -beta, -alpha);
            searched_first_move = true;
        } else {
            score = -alpha_beta(&new_position, depth - 1, -alpha - 1, -alpha);

            if (score > alpha && score < beta) {
                score = -alpha_beta(&new_position, depth - 1, -beta, -alpha);
            }
        }

        search_ply--;

        if (score > alpha) {
            tt_entry_type = ENTRY_TYPE_EXACT;
            alpha = score;
            pv_table[search_ply].moves[search_ply] = move;

            best_move = move;

            for (int next_ply = search_ply + 1; next_ply < pv_table[search_ply + 1].size; next_ply++) {
                pv_table[search_ply].moves[next_ply] = pv_table[search_ply + 1].moves[next_ply];
            }

            pv_table[search_ply].size = pv_table[search_ply + 1].size;

            if (score >= beta) {
                PackedMove best_move_packed = packed_move_new(FLAG_NORMAL, best_move.from, best_move.to);
                tt_set(position, depth, beta, best_move_packed, ENTRY_TYPE_BETA);
                return beta;
            }

        }
    }

    PackedMove best_move_packed = packed_move_new(FLAG_NORMAL, best_move.from, best_move.to);
    tt_set(position, depth, alpha, tt_entry_type, best_move_packed);

    return alpha;
}
