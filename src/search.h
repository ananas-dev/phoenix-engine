#pragma once

#include "position.h"

Move search(Position *position, double max_time_seconds);
int alpha_beta(Position *position, int depth, int alpha, int beta);