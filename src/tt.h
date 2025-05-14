#pragma once

#include <limits.h>
#include "position.h"

#define TT_MISS INT_MIN

typedef enum {
    ENTRY_TYPE_EXACT,
    ENTRY_TYPE_ALPHA,
    ENTRY_TYPE_BETA
} TTEntryType;

typedef struct TTEntry {
    uint64_t hash;
    int32_t val;
    PackedMove best_move;
    uint8_t type;
    uint8_t depth;
} TTEntry;

typedef struct {
    TTEntry *entries;
    size_t size;
} TT;

TT tt_new(int size_in_mb);
void tt_set(TT *tt, Position *position, uint8_t depth, int val, TTEntryType type, PackedMove best_move);
int tt_get(TT *tt, Position *position, uint8_t depth, int alpha, int beta, PackedMove *best_move);
double tt_fill_rate(TT *tt);
void tt_free(TT *tt);
