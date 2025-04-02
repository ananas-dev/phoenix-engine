#pragma once

#include "move.h"
#include "position.h"

void movegen_init();
void legal_moves(Position *pos, MoveList *move_list);
