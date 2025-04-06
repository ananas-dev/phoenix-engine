#include "eval.h"
#include "bitboard.h"
#include "movegen.h"

static inline int mobility(Position *position, Color color, Bitboard color_pieces, Bitboard all_pieces) {
    Bitboard soldier_mobility = BB_EMPTY;
    Bitboard general_mobility = BB_EMPTY;
    Bitboard king_mobility = BB_EMPTY;

    Bitboard soldier_it = position->pieces[color][PIECE_SOLDIER] & BB_USED;
    while (soldier_it) {
        Square from = bb_it_next(&soldier_it);
        soldier_mobility |= soldier_attacks(from) & ~color_pieces;
    }

    Bitboard general_it = position->pieces[color][PIECE_GENERAL] & BB_USED;
    while (general_it) {
        Square from = bb_it_next(&general_it);
        general_mobility |= general_attacks(from, all_pieces) & ~color_pieces;
    }

    Bitboard king_it = position->pieces[color][PIECE_KING] & BB_USED;
    while (king_it) {
        Square from = bb_it_next(&king_it);
        king_mobility |= king_attacks(from) & ~color_pieces;
    }

    return bb_popcnt(soldier_mobility) + bb_popcnt(general_mobility) + bb_popcnt(king_mobility);
}

int eval(Position *position) {
    int turn = position->side_to_move == COLOR_WHITE ? 1 : -1;

    int num_white_solider = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_SOLDIER]);
    int num_white_general = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_GENERAL]);
    int num_white_king = bb_popcnt(position->pieces[COLOR_WHITE][PIECE_KING]);
    int num_black_solider = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_SOLDIER]);
    int num_black_general = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_GENERAL]);
    int num_black_king = bb_popcnt(position->pieces[COLOR_BLACK][PIECE_KING]);

    int material_score = 900 * (num_white_general - num_black_general)
                         + 100 * (num_white_solider - num_black_solider);

    Bitboard white_pieces = pieces_by_color(position, COLOR_WHITE) & BB_USED;
    Bitboard black_pieces = pieces_by_color(position, COLOR_BLACK) & BB_USED;
    Bitboard all_pieces = white_pieces | black_pieces;

    int white_mobility = mobility(position, COLOR_WHITE, white_pieces, all_pieces);
    int black_mobility = mobility(position, COLOR_BLACK, black_pieces, all_pieces);

    int mobility_score = (white_mobility - black_mobility) * 10;

    return (material_score + mobility_score) * turn;
};

