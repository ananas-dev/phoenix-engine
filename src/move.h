#pragma once

#include "bitboard.h"

typedef struct {
    Bitboard captures;
    uint8_t from;
    uint8_t to;
} Move;

typedef struct {
    int size;
    Move moves[256];
} MoveList;

