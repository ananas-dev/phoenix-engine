#include "bitboard.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "time.h"
#include "search.h"
#include "tt.h"

State* init(void) {
    State* state = malloc(sizeof(State));
    movegen_init(state);
    position_init(state);
    eval_init(state);
    tt_init(state, 16777216);

    return state;
}

void test(void) {
}

Move act(State* state, char *position, double time_remaining) {
    Position pos = position_from_fen(position);

    position_print(&pos);

    double allocated_time = 10.0;

    // Basic time management
    if (time_remaining < 100.0) {
        allocated_time = 5.0;
    } if (time_remaining < 30.0) {
        allocated_time = 3.0;
    } else if (time_remaining < 10.0) {
        allocated_time = 1.0;
    } else if (time_remaining < 3.0) {
        allocated_time = 0.1;
    }

    return search(state, &pos, allocated_time);
}

void destroy(State *state) {
    free(state->tt);
    free(state);
}
