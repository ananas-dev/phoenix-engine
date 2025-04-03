#pragma once

typedef enum {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    NUM_SQUARE,
} Square;

typedef enum {
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    NUM_RANK,
} Rank;

typedef enum {
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H,
    NUM_FILE,
} File;

typedef enum {
    DIR_EAST = 1,
    DIR_NOEA = 9,
    DIR_NORT = 8,
    DIR_SOEA = -7,
    DIR_WEST = -1,
    DIR_SOWE = -9,
    DIR_SOUT = -8,
    DIR_NOWE = 7,
} Direction;

typedef enum {
    COLOR_WHITE,
    COLOR_BLACK,
    NUM_COLOR,
} Color;

typedef enum {
    PIECE_SOLDIER,
    PIECE_GENERAL,
    PIECE_KING,
    NUM_PIECE,
} Piece;

static inline int sq_file(Square sq) {
    return sq & 7;
}

static inline int sq_rank(Square sq) {
    return sq >> 3;
}

static inline int sq_get(File file, Rank rank) {
    return 8 * rank + file;
}

static inline void sq_to_string(Square sq, char str[3]) {
    int rank = sq_rank(sq);
    int file = sq_file(sq);

    str[0] = 'a' + (char) file;
    str[1] = '1' + (char) rank;
    str[2] = '\0';
}

