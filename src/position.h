#pragma once

#include "core.h"
#include "bitboard.h"
#include "move.h"
#include "state.h"

#define FEN_START "SSSSSS2/SSSSS2s/SSSS2ss/SSS2sss/SS2ssss/S2sssss/2ssssss 0 w 00"

typedef struct {
    Bitboard pieces[NUM_COLOR][NUM_PIECE];
    uint16_t ply;
    Color side_to_move;
    bool can_create_general;
    bool can_create_king;
    uint64_t hash;
} Position;

void position_init(State *state);

static inline bool position_is_game_over(Position *pos) {
    if (pos->ply >= 10) {
        return bb_popcnt(pos->pieces[1 - pos->side_to_move][PIECE_KING] & BB_USED) == 0;
    }

    return false;
}

static inline Bitboard pieces_by_color(Position *pos, Color color) {
    return pos->pieces[color][PIECE_SOLDIER] | pos->pieces[color][PIECE_GENERAL] | pos->pieces[color][PIECE_KING];
}

Position make_move(State *state, Position *pos, Move move);
Position position_from_fen(const char *fen_str);
void position_print(Position *pos);
