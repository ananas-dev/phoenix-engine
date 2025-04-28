#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "time.h"
#include "search.h"
#include "tt.h"

Context* init_context(bool debug) {
    Context* ctx = malloc(sizeof(Context));
    ctx->debug = debug;
    movegen_init(ctx);
    position_init(ctx);
    eval_init(ctx);
    tt_init(ctx, 67108864);

    if (debug) {
        printf("Allocated %.1lfmb for TT\n", (double)ctx->tt_size * (sizeof(TTEntry)) / (1024 * 1024));
    }


    return ctx;
}

void set_weights(Context* ctx, const int* weights, int size) {
    if (size != W_COUNT) {
        printf("Invalid number of weights !\n");
        exit(1);
    }
    memcpy(ctx->weights, weights, W_COUNT * sizeof(int));
}

State *new_game(Context *ctx, State *state) {
    if (state == NULL) {
        state = malloc(sizeof(State));
    }

    // Full reset
    memset(ctx->tt, 0, ctx->tt_size * sizeof(TTEntry));
    memset(state, 0, sizeof(State));
    state->ctx = ctx;

    return state;
}

MoveWithMateInfo act(State* state, const char *position, double time_remaining, bool irreversible) {
    (void) time_remaining;
    Position pos = position_from_fen(state->ctx, position);

    if (irreversible || state->game_history.size >= 1024) {
        list_clear(&state->game_history);
    }

    list_push(&state->game_history, pos.hash);

    if (state->ctx->debug) {
        position_print(&pos);
    }

    double allocated_time = time_remaining / 30;
    // double allocated_time = 0.1;

    MoveWithMateInfo search_result = search(state, &pos, allocated_time);

    if (state->ctx->debug) {
        printf("Transposition Table fill rate: %.2f%%\n", tt_fill_rate(state->ctx));
    }

    return search_result;
}

void destroy_state(State *state) {
    free(state);
}

void destroy_context(Context *ctx) {
    free(ctx->tt);
    free(ctx);
}

#ifdef EMSCRIPTEN
#include <emscripten.h>

int main() {
    return 0;
}
#endif


// double sigmoid(double s, double k) {
//     return 1.0 / (1.0 + exp(-s * log(10.0) * k / 400.0));
// }

// void load_position_db(State *state, const char *file_name) {
// }
//
// double evaluation_error(State *state, double k) {
//     double sum = 0.0;
//     for (uint64_t i = 0; i < state->position_db.size; i++) {
//         Position *position = &state->position_db.positions[i];
//         double q_i = position->side_to_move == COLOR_WHITE
//                      ? eval(state, position)
//                      : -eval(state, position);
//
//         double error = state->position_db.results[i] - sigmoid(q_i, k);
//         sum += error * error;
//
//     }
//
//     return sum / (double)state->position_db.size;
// }

