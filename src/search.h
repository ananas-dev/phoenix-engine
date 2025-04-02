#pragma once

#include "position.h"

Move search(Position *position, int depth);
int alpha_beta(Position *position, int depth, int alpha, int beta);