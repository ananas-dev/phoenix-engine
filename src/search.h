#pragma once

#include "position.h"
#include "state.h"

MoveWithMateInfo search(State *state, Position *position, double max_time_seconds);
