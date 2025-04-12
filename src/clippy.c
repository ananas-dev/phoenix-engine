#include "bitboard.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "time.h"
#include "search.h"
#include "tt.h"

State* init(bool debug) {
    State* state = malloc(sizeof(State));
    state->debug = debug;
    movegen_init(state);
    position_init(state);
    eval_init(state);
    tt_init(state, 16777216);

    return state;
}

void set_weights(State* state, const int* weights, int size) {
    if (size != W_COUNT) {
        printf("Invalid number of weights !\n");
        exit(1);
    }
    memcpy(state->weights, weights, W_COUNT * sizeof(int));
}

void new_game(State *state) {
    memset(state->tt, 0, sizeof(Entry) * state->tt_size);
}

MoveWithMateInfo act(State* state, const char *position, double time_remaining) {
    (void) time_remaining;
    Position pos = position_from_fen(position);

    // Freeze this position to a zero score to avoid repetitions
    tt_freeze(state, &pos);

    if (state->debug) {
        position_print(&pos);
    }

    double allocated_time = 0.1;

    // Basic time management
    // if (time_remaining < 100.0) {
    //     allocated_time = 5.0;
    // } if (time_remaining < 30.0) {
    //     allocated_time = 3.0;
    // } else if (time_remaining < 10.0) {
    //     allocated_time = 1.0;
    // } else if (time_remaining < 3.0) {
    //     allocated_time = 0.1;
    // }

    return search(state, &pos, allocated_time);
}

int static_eval_position(State *state, const char *position) {
    Position pos = position_from_fen(position);
    return eval(state, &pos);
}

double sigmoid(double s) {
    return 1.0 / (1.0 + pow(10.0, -4.0 * s / 400.0));
}

double evaluation_error(State *state, const char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    double sum = 0.0;
    uint64_t N = 0;

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        // Remove trailing newline
        buffer[strcspn(buffer, "\r\n")] = 0;

        // Parse FEN and score
        char *fen_str = strtok(buffer, ",");
        char *score_str = strtok(NULL, ",");

        if (fen_str == NULL || score_str == NULL) {
            fprintf(stderr, "Malformed line, skipping: %s\n", buffer);
            continue;
        }

        double R_i;
        if (sscanf(score_str, "%lf", &R_i) != 1) {
            fprintf(stderr, "Failed to parse score: %s\n", score_str);
            continue;
        }

        Position position = position_from_fen(fen_str);
        double q_i = position.side_to_move == COLOR_WHITE
                     ? eval(state, &position)
                     : -eval(state, &position);

        double error = R_i - sigmoid(q_i);
        sum += error * error;
        N++;
    }

    fclose(file);

    if (N == 0) {
        fprintf(stderr, "No valid lines parsed.\n");
        return 0.0; // or NAN or error value
    }

    return sum / (double)N;
}

void destroy(State *state) {
    free(state->tt);
    free(state);
}
