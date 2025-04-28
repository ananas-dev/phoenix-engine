#pragma once

#include "core.h"
#include "bitboard.h"
#include "move.h"
#include "context.h"
#include "nnue.h"

#define FEN_START "SSSSSS2/SSSSS2s/SSSS2ss/SSS2sss/SS2ssss/S2sssss/2ssssss 0 w 00"

typedef struct {
    Bitboard pieces[NUM_COLOR][NUM_PIECE];
    Accumulator accumulators[NUM_COLOR];
    uint64_t hash;
    uint16_t ply;
    uint16_t half_move_clock;
    Color side_to_move;
    bool can_create_general;
    bool can_create_king;
} Position;

void position_init(Context *ctx);

static inline GameState position_state(Position *pos) {
    if (pos->half_move_clock >= 50) {
        return STATE_DRAW;
    }

    if (pos->ply >= 10 && bb_popcnt(pos->pieces[1 - pos->side_to_move][PIECE_KING] & BB_USED) == 0) {
        return STATE_WIN;
    }

    Bitboard *white_pieces = pos->pieces[COLOR_WHITE];
    Bitboard *black_pieces = pos->pieces[COLOR_BLACK];
    bool no_white_piece = bb_is_empty(white_pieces[PIECE_SOLDIER] | white_pieces[PIECE_GENERAL] | white_pieces[PIECE_KING]);
    bool no_black_piece = bb_is_empty(black_pieces[PIECE_SOLDIER] | black_pieces[PIECE_GENERAL] | black_pieces[PIECE_KING]);

    if (no_white_piece && no_black_piece) {
        return STATE_DRAW;
    }

    if (no_white_piece) {
        return pos->side_to_move == COLOR_WHITE ? STATE_LOSS : STATE_WIN;
    }

    if (no_black_piece) {
        return pos->side_to_move == COLOR_BLACK ? STATE_LOSS : STATE_WIN;
    }

    return STATE_ONGOING;
}

static inline Bitboard pieces_by_color(Position *pos, Color color) {
    return pos->pieces[color][PIECE_SOLDIER] | pos->pieces[color][PIECE_GENERAL] | pos->pieces[color][PIECE_KING];
}

static inline int count_pieces(Position *pos, Piece piece_type) {
    return bb_popcnt(pos->pieces[COLOR_WHITE][piece_type]) + bb_popcnt(pos->pieces[COLOR_BLACK][piece_type]);
}

Position make_move(Context *ctx, Position *pos, Move move);
Position position_from_fen(Context *ctx, const char *fen_str);
Position position_from_fen_no_hash(const char *fen_str);
uint64_t position_hash(Context *ctx, const Position *position);

void position_print(Position *pos);

