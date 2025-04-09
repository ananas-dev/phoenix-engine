#pragma once

#include "move.h"
#include "position.h"
#include "state.h"

void movegen_init(State *state);
void legal_moves(State *state, Position *pos, MoveList *move_list);
uint64_t perft(State *state, Position *position, int depth, bool debug);
Bitboard general_attacks(State *state, Square sq, Bitboard blockers);
Bitboard king_attacks(State *state, Square sq);
Bitboard soldier_attacks(State *state, Square sq);
