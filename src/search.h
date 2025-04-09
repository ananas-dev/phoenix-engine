#pragma once

#include "position.h"
#include "state.h"

Move search(State *state, Position *position, double max_time_seconds);
