#pragma once

#include "move.h"
#include "position.h"

void movegen_init(Context *ctx);
void legal_moves(Context *ctx, Position *pos, MoveList *move_list);
uint64_t perft(Context *ctx, Position *position, int depth, bool debug);
Bitboard general_attacks(Context *ctx, Square sq, Bitboard blockers);
Bitboard king_attacks(Context *ctx, Square sq);
Bitboard soldier_attacks(Context *ctx, Square sq);
