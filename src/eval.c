#include "eval.h"
#include "bitboard.h"
#include "movegen.h"

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



    // Counter pieces on each corner
    int num_white_soldier_on_corners = 0;
    int num_black_soldier_on_corners = 0;

    num_white_soldier_on_corners += bb_popcnt(position->pieces[COLOR_WHITE][PIECE_SOLDIER] & BB_ALL_CORNERS);
    num_black_soldier_on_corners += bb_popcnt(position->pieces[COLOR_BLACK][PIECE_SOLDIER] & BB_ALL_CORNERS);

    int corners_score = 10 * (num_white_soldier_on_corners - num_black_soldier_on_corners);


    // MoveList move_list = {0};
    // legal_moves(position, &move_list);
    // int num_white_legal_move = move_list.size;
    //
    // Position black_pov = make_null_move(position);
    // move_list.size = 0;
    // legal_moves(&black_pov, &move_list);
    // int num_black_legal_move = move_list.size;
    //
    // int mobility_score = 5 * (num_white_legal_move - num_black_legal_move);
    //
    // if (position->ply < 10) {
    //     return mobility_score * turn;
    // }
    //
    // return (material_score + mobility_score) * turn;
    return (material_score + corners_score) * turn;
};

