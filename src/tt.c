#include "tt.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static size_t floor_power_of_two(size_t x) {
    if (x == 0) return 0;
    size_t power = 1;
    while (power <= x) power <<= 1;
    return power >> 1;
}

TT tt_new(int size_in_mb) {
    size_t total_bytes = size_in_mb * 1024 * 1024;
    size_t max_entries = total_bytes / sizeof(TTEntry);
    size_t bucket_count = floor_power_of_two(max_entries);

    TTEntry *entry = malloc(sizeof(TTEntry) * bucket_count);

    if (!entry) {
        exit(1);
    }

    return (TT) {
        .entries = entry,
        .size = bucket_count,
    };
}

void tt_set(TT *tt, Position *position, uint8_t depth, int val, TTEntryType type, PackedMove best_move) {
    TTEntry *entry = &tt->entries[position->hash & (tt->size - 1)];

    if ((position->hash == entry->hash) && (depth <= entry->depth)) return;

    *entry = (TTEntry) {
        .hash = position->hash,
        .depth = depth,
        .val = val,
        .type = type,
        .best_move = best_move,
    };
}

int tt_get(TT *tt, Position *position, uint8_t depth, int alpha, int beta, PackedMove *best_move) {
    TTEntry *entry = &tt->entries[position->hash & (tt->size - 1)];

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

// double tt_fill_rate(Context *ctx) {
//     uint32_t filled_entries = 0;
//     uint32_t total_entries = ctx->tt_size;
//
//     for (size_t i = 0; i < total_entries; i++) {
//         if (ctx->tt[i].hash != 0) {
//             filled_entries++;
//         }
//     }
//
//     return ((double)filled_entries / (double)total_entries) * 100.0f;
// }

void tt_free(TT *tt) {
    free(tt->entries);
}
