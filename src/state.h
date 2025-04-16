#pragma once

#include <sys/time.h>
#include "position.h"

#define MAX_PLY 128
#define PV_TABLE_SIZE 64
#define MAX_HISTORY 1000

typedef struct {
    Move moves[PV_TABLE_SIZE];
    int size;
} PVLine;

typedef struct {
    Context *ctx;

    Position game_history[1024];
    // Search state
    int nodes_visited;
    struct timeval start_time;
    double max_time;
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
} State;

