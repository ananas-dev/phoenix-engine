#pragma once

#include <limits.h>
#include "bitboard.h"
#include "position.h"

#define TT_MISS INT_MIN

void tt_init(State *state, int size);
void tt_freeze(State *state, Position *position);
void tt_set(State *state, Position *position, uint8_t depth, int val, EntryType type, PackedMove best_move);
int tt_get(State *state, Position *position, uint8_t depth, int alpha, int beta, PackedMove *best_move);
