#pragma once

#include "bitboard.h"

typedef struct {
    Bitboard captures;
    uint8_t from;
    uint8_t to;
} Move;

typedef struct {
    int size;
    Move elems[128];
} MoveList;

typedef enum {
    FLAG_NULL = 0,
    FLAG_NORMAL = 1,
} PackedMoveFlag;

// Flag (4 bits) + From (6 bits) + To (6 bits)
typedef uint16_t PackedMove;

#define NULL_PACKED_MOVE 0

static inline PackedMove packed_move_new(PackedMoveFlag flag, Square from, Square to) {
    return ((PackedMove) flag << 12) | ((PackedMove) from << 6) | ((PackedMove) to);
}

static inline void packed_move_extract(PackedMove move, PackedMoveFlag *flag, Square *from, Square *to) {
    *flag = (PackedMoveFlag) ((move >> 12) & 0xF); // 4 bits
    *from = (Square) ((move >> 6) & 0x3F); // 6 bits
    *to = (Square) (move & 0x3F); // 6 bits
}

void move_print(Move move);
char* move_to_uci(Move move);
Move uci_to_move(const char* uci);
