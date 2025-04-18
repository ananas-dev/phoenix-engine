#pragma once

#include <limits.h>
#include "bitboard.h"
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

void tt_init(Context *ctx, int size);
void tt_set(Context *ctx, Position *position, uint8_t depth, int val, TTEntryType type, PackedMove best_move);
int tt_get(Context *ctx, Position *position, uint8_t depth, int alpha, int beta, PackedMove *best_move);
double tt_fill_rate(Context *ctx);
