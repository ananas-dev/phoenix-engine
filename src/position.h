#pragma once

#include "core.h"
#include "bitboard.h"
#include "move.h"

typedef struct {
    Bitboard pieces[NUM_COLOR][NUM_PIECE];
    uint16_t ply;
    Color side_to_move;
    bool can_create_general;
    bool can_create_king;
} Position;



Position make_move(Position *pos, Move move);
Position position_from_fen(const char *fen_str);
void position_print(Position *pos);
