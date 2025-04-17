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
    tt_init(ctx, 16777216);

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

    memset(ctx->tt, 0, ctx->tt_size * sizeof(TTEntry));
    list_clear(&state->game_history);

    state->ctx = ctx;

    return state;
}

MoveWithMateInfo act(State* state, const char *position, double time_remaining, bool irreversible) {
    (void) time_remaining;
    Position pos = position_from_fen(state->ctx, position);

    if (irreversible) {
        list_clear(&state->game_history);
    }

    list_push(&state->game_history, pos.hash);

    if (state->ctx->debug) {
        position_print(&pos);
    }

    double allocated_time = time_remaining / 30;

    return search(state, &pos, allocated_time);
}

void destroy_state(State *state) {
    free(state);
}

void destroy_context(Context *ctx) {
    free(ctx->tt);
    free(ctx);
}

double sigmoid(double s, double k) {
    return 1.0 / (1.0 + exp(-s * log(10.0) * k / 400.0));
}

// void load_position_db(State *state, const char *file_name) {
//     FILE *file = fopen(file_name, "r");
//     if (file == NULL) {
//         perror("Error opening file");
//         exit(1);
//     }
//
//     // First pass: count number of lines
//     uint64_t line_count = 0;
//     char buffer[1024];
//     while (fgets(buffer, sizeof(buffer), file)) {
//         if (buffer[0] != '\n' && buffer[0] != '\r') {
//             line_count++;
//         }
//     }
//
//     // Allocate exact size
//     state->position_db.positions = malloc(line_count * sizeof(Position));
//     state->position_db.results = malloc(line_count * sizeof(double));
//     if (!state->position_db.positions || !state->position_db.results) {
//         fprintf(stderr, "Memory allocation failed\n");
//         exit(1);
//     }
//
//     // Second pass: parse data
//     rewind(file);
//     uint64_t N = 0;
//
//     while (fgets(buffer, sizeof(buffer), file)) {
//         buffer[strcspn(buffer, "\r\n")] = 0;
//
//         char *fen_str = strtok(buffer, ",");
//         char *score_str = strtok(NULL, ",");
//
//         if (fen_str == NULL || score_str == NULL) {
//             fprintf(stderr, "Malformed line, skipping: %s\n", buffer);
//             continue;
//         }
//
//         double R_i;
//         if (sscanf(score_str, "%lf", &R_i) != 1) {
//             fprintf(stderr, "Failed to parse score: %s\n", score_str);
//             continue;
//         }
//
//         Position pos = position_from_fen(fen_str);
//
//         // Skip positions with a missing king because they don't matter
//         if (bb_popcnt(pos.pieces[COLOR_WHITE][PIECE_KING]) == 0 || bb_popcnt(pos.pieces[COLOR_BLACK][PIECE_KING]) == 0) {
//             continue;
//         }
//
//         state->position_db.positions[N] = pos;
//         state->position_db.results[N] = R_i;
//         N++;
//     }
//
//     fclose(file);
//     state->position_db.size = N;  // Store actual count if useful
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

