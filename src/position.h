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
    uint64_t hash;
} Position;

void position_init();

static inline bool position_is_game_over(Position *pos) {
    if (pos->ply >= 10) {
        return !pos->can_create_king && bb_popcnt(pos->pieces[pos->side_to_move][PIECE_KING] & BB_USED) == 0;
    }

    return false;
}

Position make_move(Position *pos, Move move);
Position make_null_move(Position *pos);
Position position_from_fen(const char *fen_str);
void position_print(Position *pos);
