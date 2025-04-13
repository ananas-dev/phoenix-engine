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

typedef struct {
    uint64_t hash[50];
    int size;
} HashList;

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

typedef enum {
    // tables
    W_OP_SOLDIER_PIECE_TABLE = 0,
    W_OP_GENERAL_PIECE_TABLE = NUM_SQUARE,
    W_OP_KING_PIECE_TABLE = NUM_SQUARE * 2,
    W_EG_SOLDIER_PIECE_TABLE = NUM_SQUARE * 3,
    W_EG_GENERAL_PIECE_TABLE = NUM_SQUARE * 4,
    W_EG_KING_PIECE_TABLE = NUM_SQUARE * 5,

    W_OP_GENERAL_MATERIAL = NUM_SQUARE * 6,
    W_OP_SOLDIER_MOBILITY,
    W_OP_SOLDIER_KING_DIST,
    W_OP_GENERAL_MOBILITY,
    W_OP_KING_MOBILITY,
    W_OP_KING_SHELTER,
    W_OP_KING_THREATS,
    W_OP_SS_PAIR,
    W_OP_SG_PAIR,
    W_OP_SQUARE_STRUCTURES,
    W_OP_CONTROL,

    W_EG_SOLDIER_MATERIAL,
    W_EG_GENERAL_MATERIAL,
    W_EG_SOLDIER_MOBILITY,
    W_EG_SOLDIER_KING_DIST,
    W_EG_GENERAL_MOBILITY,
    W_EG_KING_MOBILITY,
    W_EG_KING_SHELTER,
    W_EG_KING_THREATS,
    W_EG_KING_CHASE,
    W_EG_SS_PAIR,
    W_EG_SG_PAIR,
    W_EG_SQUARE_STRUCTURES,
    W_EG_CONTROL,

    W_COUNT  // Always a good idea to have a count for loops etc.
} Weight;

typedef struct {
    bool debug;

    // Eval
    int weights[W_COUNT];

    Bitboard square_structure_table[NUM_FILE - 1][NUM_RANK - 1];

    // Position state
    Zobrists zobrists;

    // Movegen state
    Magic magics[NUM_SQUARE];
    Bitboard general_attack_table[NUM_SQUARE * 4096];

    Bitboard soldier_attack_table[NUM_SQUARE];
    Bitboard king_attack_table[NUM_SQUARE];

    HashList hash_list;

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

