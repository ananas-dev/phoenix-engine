#pragma once

#include <limits.h>
#include "bitboard.h"
#include "position.h"

#define TT_MISS INT_MIN

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

void tt_init(int size);
void tt_set(Position *position, uint8_t depth, int val, EntryType type, PackedMove best_move);
int tt_get(Position *position, uint8_t depth, int alpha, int beta, PackedMove *best_move);
