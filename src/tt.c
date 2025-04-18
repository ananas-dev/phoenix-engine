#include "tt.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void tt_init(Context *ctx, int size) {
    TTEntry *tt;

    int attempt = size;
    do {
        tt = malloc(sizeof(TTEntry) * attempt);
        if (!tt) attempt /= 2;
    } while (tt == NULL && attempt > 0);

    ctx->tt = tt;
    ctx->tt_size = attempt;
}

void tt_set(Context *ctx, Position *position, uint8_t depth, int val, TTEntryType type, PackedMove best_move) {
    TTEntry *entry = &ctx->tt[position->hash & (ctx->tt_size - 1)];

    if ((position->hash == entry->hash) && (depth <= entry->depth)) return;

    *entry = (TTEntry) {
        .hash = position->hash,
        .depth = depth,
        .val = val,
        .type = type,
        .best_move = best_move,
    };
}

int tt_get(Context *ctx, Position *position, uint8_t depth, int alpha, int beta, PackedMove *best_move) {
    TTEntry *entry = &ctx->tt[position->hash & (ctx->tt_size - 1)];

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

double tt_fill_rate(Context *ctx) {
    uint32_t filled_entries = 0;
    uint32_t total_entries = ctx->tt_size;

    for (size_t i = 0; i < total_entries; i++) {
        if (ctx->tt[i].hash != 0) {
            filled_entries++;
        }
    }

    return ((double)filled_entries / (double)total_entries) * 100.0f;
}

void tt_free(TTEntry *tt) {
    free(tt);
}
