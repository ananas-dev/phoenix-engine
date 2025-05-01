#pragma once

#include <sys/time.h>
#include "position.h"
#include "nnue.h"
#include "tt.h"

#define MAX_PLY 128
#define PV_TABLE_SIZE 64
#define MAX_HISTORY 1000

typedef struct {
    Move elems[PV_TABLE_SIZE];
    int size;
} PVLine;

typedef struct {
    uint64_t elems[1024];
    int size;
} GameHistory;

typedef struct {
    bool debug;

    _Atomic(bool) stopped;

    GameHistory game_history;

    Network net;

    // Search state
    int nodes_visited;
    struct timeval start_time;
    int64_t max_time;
    bool time_over;

    // Move ordering state
    PackedMoveFlag tt_best_move_flag;
    Square tt_best_move_from;
    Square tt_best_move_to;
    bool tt_best_move_sorting;

    // Principal variation table
    PVLine pv_table[MAX_PLY];
    bool pv_sorting;
    bool follow_pv;

    // Move ordering heuristics
    Move killer_moves[2][MAX_PLY];
    int history[NUM_COLOR][NUM_SQUARE][NUM_SQUARE];

    // Current search depth
    int search_ply;

    // Current position
    Position position;

    TT tt;
} State;

