#include "tt.h"
#include <stdlib.h>
#include <assert.h>

void tt_init(Context *ctx, int size) {
    TTEntry *tt = malloc(sizeof(TTEntry) * size);
    assert(tt != NULL && "Could not init transposition table");
    ctx->tt = tt;
    ctx->tt_size = size;
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

// Hacky repetition checker
void tt_freeze(Context *ctx, Position *position) {
    TTEntry *entry = &ctx->tt[position->hash & (ctx->tt_size - 1)];

    *entry = (TTEntry) {
        .hash = position->hash,
        .depth = 255,
        .val = 0,
        .type = ENTRY_TYPE_EXACT,
        .best_move = NULL_PACKED_MOVE,
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

void tt_free(TTEntry *tt) {
    free(tt);
}
