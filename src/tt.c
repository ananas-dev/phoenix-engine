#include "tt.h"
#include <stdlib.h>
#include <assert.h>

Entry *tt;
int tt_size;

void tt_init(int size) {
    tt = malloc(sizeof(Entry) * size);
    tt_size = size;
    assert(tt != NULL && "Could not init transposition table");
}

void tt_set(Position *position, uint8_t depth, int val, EntryType type) {
    Entry *entry = &tt[position->hash & (tt_size - 1)];

    if ((position->hash == entry->hash) && (depth < entry->depth)) return;

    tt[position->hash & (tt_size - 1)] = (Entry) {
        .hash = position->hash,
        .depth = depth,
        .val = val,
        .type = type,
    };
}

int tt_get(Position *position, uint8_t depth, int alpha, int beta) {
    Entry *entry = &tt[position->hash & (tt_size - 1)];

    if (position->hash == entry->hash) {
        if (entry->depth >= depth) {
            if (entry->type == ENTRY_TYPE_EXACT) {
                return entry->val;
            }

            if ((entry->type == ENTRY_TYPE_ALPHA) && (entry->val < alpha)) {
                return alpha;
            }

            if ((entry->type == ENTRY_TYPE_BETA) && (entry->val >= beta)) {
                return beta;
            }
        }
    }

    return TT_MISS;
}

void tt_free() {
    free(tt);
}
