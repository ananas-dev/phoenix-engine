#pragma once

#include "core.h"

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t Bitboard;

#define BB_USED               0x00FFFFFFFFFFFFFLL
#define BB_EMPTY              0x00000000000000ULL
#define BB_FULL               0xFFFFFFFFFFFFFFULL
#define BB_NOT_FIRST_RANK     0x00FFFFFFFFFFFFULL
#define BB_NOT_SEVENTH_RANK   0xFFFFFFFFFFFF00ULL
#define BB_A1_H8_DIAGONAL     0x80402010080402ULL
#define BB_H1_A8_ANTIDIAGONAL 0x01020408102040ULL
#define BB_LIGHT_SQUARES      0x55AA55AA55AA55ULL
#define BB_DARK_SQUARES       0xAA55AA55AA55AAULL
#define BB_NOT_A_FILE         0xFEFEFEFEFEFEFEULL
#define BB_NOT_H_FILE         0x7F7F7F7F7F7F7FULL
#define BB_NOT_EDGE           (BB_NOT_A_FILE & BB_NOT_H_FILE & BB_NOT_FIRST_RANK & BB_NOT_SEVENTH_RANK)

bool bb_is_empty(Bitboard bb);
Square bb_it_next(Bitboard* bb);
Bitboard bb_from_sq(Square sq);
int bb_popcnt(Bitboard x);
void bb_print(Bitboard bb);