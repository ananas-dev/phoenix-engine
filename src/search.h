#pragma once

#include "position.h"
#include "state.h"

typedef struct {
    Move best_move;
    int32_t score;
} SearchResult;

SearchResult search(State *state);
