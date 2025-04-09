#include "tt.h"
#include <stdlib.h>
#include <assert.h>

void tt_init(State *state, int size) {
    Entry *tt = malloc(sizeof(Entry) * size);
    assert(tt != NULL && "Could not init transposition table");
    state->tt = tt;
    state->tt_size = size;
}

void tt_set(State *state, Position *position, uint8_t depth, int val, EntryType type, PackedMove best_move) {
    Entry *entry = &state->tt[position->hash & (state->tt_size - 1)];

    if ((position->hash == entry->hash) && (depth <= entry->depth)) return;

    *entry = (Entry) {
        .hash = position->hash,
        .depth = depth,
        .val = val,
        .type = type,
        .best_move = best_move,
    };
}

int tt_get(State *state, Position *position, uint8_t depth, int alpha, int beta, PackedMove *best_move) {
    Entry *entry = &state->tt[position->hash & (state->tt_size - 1)];

    if (position->hash == entry->hash) {
        *best_move = entry->best_move;

        if (entry->depth >= depth) {
            if (entry->type == ENTRY_TYPE_EXACT) {
                return entry->val;
            }

            if ((entry->type == ENTRY_TYPE_ALPHA) && (entry->val <= alpha)) {
                return alpha;
            }

            if ((entry->type == ENTRY_TYPE_BETA) && (entry->val >= beta)) {
                return beta;
            }
        }
    }

    return TT_MISS;
}

void tt_free(Entry *tt) {
    free(tt);
}
