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
    uint8_t depth;
} Entry;

void tt_init(int size);
void tt_set(Position *position, uint8_t depth, int val, EntryType type);
int tt_get(Position *position, uint8_t depth, int alpha, int beta);
