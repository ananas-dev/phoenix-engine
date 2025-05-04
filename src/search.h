#pragma once

#include "position.h"
#include "state.h"

typedef struct {
    Move best_move;
    bool forced_win;
} SearchResult;

SearchResult search(State *state);
