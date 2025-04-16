#pragma once

#include <stdbool.h>
#include "core.h"
#include "bitboard.h"

struct TTEntry;

typedef struct {
    Bitboard *ptr;
    Bitboard mask;
    uint64_t magic;
    uint8_t offset;
} Magic;

typedef struct {
    uint64_t pieces[NUM_COLOR][NUM_PIECE][NUM_SQUARE];
    uint64_t side_to_move;
    uint64_t can_create_general;
    uint64_t can_create_king;
} Zobrists;

typedef enum {
    // tables
    W_OP_SOLDIER_PIECE_TABLE = 0,
    W_OP_GENERAL_PIECE_TABLE = NUM_SQUARE,
    W_OP_KING_PIECE_TABLE = NUM_SQUARE * 2,
    W_EG_SOLDIER_PIECE_TABLE = NUM_SQUARE * 3,
    W_EG_GENERAL_PIECE_TABLE = NUM_SQUARE * 4,
    W_EG_KING_PIECE_TABLE = NUM_SQUARE * 5,

    W_OP_SOLDIER_MATERIAL = NUM_SQUARE * 6,
    W_OP_GENERAL_MATERIAL,
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

    W_COUNT
} Weight;

// typedef struct {
//     Position *positions;
//     double *results;
//     uint64_t size;
// } PositionDB;


typedef struct {
    bool debug;

    // Eval
    int weights[W_COUNT];

    Bitboard square_structure_table[NUM_FILE - 1][NUM_RANK - 1];
    int distance[NUM_SQUARE][NUM_SQUARE];

    // Position state
    Zobrists zobrists;

    // Movegen state
    Magic magics[NUM_SQUARE];
    Bitboard general_attack_table[NUM_SQUARE * 4096];

    Bitboard soldier_attack_table[NUM_SQUARE];
    Bitboard king_attack_table[NUM_SQUARE];

    struct TTEntry *tt;
    int tt_size;

} Context;

