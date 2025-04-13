#pragma once

#include "core.h"
#include "bitboard.h"
#include "move.h"
#include "state.h"

#define FEN_START "SSSSSS2/SSSSS2s/SSSS2ss/SSS2sss/SS2ssss/S2sssss/2ssssss 0 w 00"

struct Position {
    Bitboard pieces[NUM_COLOR][NUM_PIECE];
    uint64_t hash;
    uint16_t ply;
    uint16_t half_move_clock;
    Color side_to_move;
    bool can_create_general;
    bool can_create_king;
};

void position_init(State *state);

static inline GameState position_state(Position *pos) {
    if (pos->half_move_clock >= 50) {
        return STATE_DRAW;
    }

    if (pos->ply >= 10 && bb_popcnt(pos->pieces[1 - pos->side_to_move][PIECE_KING] & BB_USED) == 0) {
        return STATE_WIN;
    }

    return STATE_ONGOING;
}

static inline Bitboard pieces_by_color(Position *pos, Color color) {
    return pos->pieces[color][PIECE_SOLDIER] | pos->pieces[color][PIECE_GENERAL] | pos->pieces[color][PIECE_KING];
}

static inline int count_pieces(Position *pos, Piece piece_type) {
    return bb_popcnt(pos->pieces[COLOR_WHITE][piece_type]) + bb_popcnt(pos->pieces[COLOR_BLACK][piece_type]);
}

Position make_move(State *state, Position *pos, Move move);
Position position_from_fen(const char *fen_str);
void position_print(Position *pos);

