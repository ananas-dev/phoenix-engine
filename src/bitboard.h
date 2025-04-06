#pragma once

#include "core.h"

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t Bitboard;

#define BB_USED               0xFFFFFFFFFFFFFFULL
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
#define BB_TOP_LEFT_CORNER    0x01000000000000ULL
#define BB_TOP_RIGHT_CORNER   0x80000000000000ULL
#define BB_BOTTOM_LEFT_CORNER 0x00000000000001ULL
#define BB_BOTTOM_RIGHT_CORNER 0x0000000000080ULL
#define BB_ALL_CORNERS        0x81000000000081ULL

void bb_print(Bitboard bb);

static inline __attribute__((always_inline)) bool bb_is_empty(Bitboard bb) {
  return (bb & BB_USED) == BB_EMPTY;
}

static inline __attribute__((always_inline)) Bitboard bb_from_sq(Square sq) {
  return 1ULL << sq;
};

static inline __attribute__((always_inline)) Square bb_it_next(Bitboard* b) {
  Square s = __builtin_ctzll(*b);
  *b &= *b - 1;
  return s;
}

static inline __attribute__((always_inline)) int bb_popcnt(Bitboard x) {
  return __builtin_popcountll(x);
}

