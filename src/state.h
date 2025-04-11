#pragma once

#include <sys/time.h>

#define MAX_PLY 128
#define PV_TABLE_SIZE 64
#define MAX_HISTORY 1000

typedef struct {
    Bitboard *ptr;
    Bitboard mask;
    uint64_t magic;
    uint8_t offset;
} Magic;

typedef struct {
    Move moves[PV_TABLE_SIZE];
    int size;
} PVLine;

typedef struct {
    uint64_t pieces[NUM_COLOR][NUM_PIECE][NUM_SQUARE];
    uint64_t side_to_move;
    uint64_t can_create_general;
    uint64_t can_create_king;
} Zobrists;

typedef enum {
    ENTRY_TYPE_EXACT,
    ENTRY_TYPE_ALPHA,
    ENTRY_TYPE_BETA
} EntryType;

typedef struct {
    Bitboard hash;
    EntryType type;
    int val;
    PackedMove best_move;
    uint8_t depth;
} Entry;

typedef struct {
    int op_general_material;
    int op_soldier_mobility;
    int op_soldier_center;
    int op_soldier_king_dist;
    int op_general_mobility;
    int op_king_mobility;
    int op_king_shelter;
    int op_king_center;
    int op_king_threats;
    int op_ss_pair;
    int op_sg_pair;
    int op_square_structures;
    int op_edge_pieces;
    int op_control;

    int eg_soldier_material;
    int eg_general_material;
    int eg_soldier_mobility;
    int eg_soldier_center;
    int eg_soldier_king_dist;
    int eg_general_mobility;
    int eg_king_mobility;
    int eg_king_shelter;
    int eg_king_center;
    int eg_king_threats;
    int eg_king_chase;
    int eg_ss_pair;
    int eg_sg_pair;
    int eg_square_structures;
    int eg_edge_pieces;
    int eg_control;
} EvalWeights;

typedef struct {
    bool debug;

    // Eval
    EvalWeights weights;

    Bitboard square_structure_table[NUM_FILE - 1][NUM_RANK - 1];

    // Position state
    Zobrists zobrists;

    // Movegen state
    Magic magics[NUM_SQUARE];
    Bitboard general_attack_table[NUM_SQUARE * 4096];

    Bitboard soldier_attack_table[NUM_SQUARE];
    Bitboard king_attack_table[NUM_SQUARE];

    Entry *tt;
    int tt_size;

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

    // Eval tables
    int distance[NUM_SQUARE][NUM_SQUARE];
} State;

