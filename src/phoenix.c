#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitboard.h"

typedef struct {
    Bitboard captured;
    Square from;
    Square to;
} Move;

typedef struct {
    int size;
    Move moves[256];
} MoveList;

typedef struct {
    Bitboard pieces[NUM_COLOR][NUM_PIECE];
    uint16_t ply;
    Color side_to_move;
    bool can_create_general;
    bool can_create_king;
} Position;

void sq_to_string(Square sq, char str[3]) {
    int rank = sq_rank(sq);
    int file = sq_file(sq);

    str[0] = 'a' + (char) file;
    str[1] = '1' + (char) rank;
    str[2] = '\0';
}

void position_print(Position *pos) {
    printf("+---+---+---+---+---+---+---+---+\n");
    for (int rank = 6; rank >= 0; --rank) {
        for (int file = 0; file <= 7; file++) {
            Bitboard square_mask = bb_from_sq(sq_get(file, rank));

            Piece current_piece = -1;
            Color current_color = -1;

            for (Color color = COLOR_WHITE; color <= COLOR_BLACK; color++) {
                for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
                    if (pos->pieces[color][piece] & square_mask) {
                        current_piece = piece;
                        current_color = color;
                        goto end;
                    }
                }
            }
        end:

            if (current_piece == -1) {
                printf("|   ");
                continue;
            }

            if (current_color == COLOR_WHITE) {
                switch (current_piece) {
                    case PIECE_SOLDIER:
                        printf("| S ");
                        break;
                    case PIECE_GENERAL:
                        printf("| G ");
                        break;
                    case PIECE_KING:
                        printf("| K ");
                        break;
                    default:
                        assert(false && "Invalid piece");
                }
            } else {
                switch (current_piece) {
                    case PIECE_SOLDIER:
                        printf("| s ");
                        break;
                    case PIECE_GENERAL:
                        printf("| g ");
                        break;
                    case PIECE_KING:
                        printf("| k ");
                        break;
                    default:
                        assert(false && "Invalid piece");
                }
            }
        }
        printf("| %d\n", rank + 1);
        printf("+---+---+---+---+---+---+---+---+\n");
    }
    printf("  a   b   c   d   e   f   g   h\n");
}

uint64_t random_u64() {
    static uint64_t number = 0xFFAAB58C5833FE89;

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    return number;
}

typedef struct {
    Bitboard *ptr;
    Bitboard mask;
    uint64_t magic;
    uint8_t offset;
} Magic;

Magic magics[NUM_SQUARE];
Bitboard general_attack_table[NUM_SQUARE * 4096];

Bitboard soldier_attack_table[NUM_SQUARE];
Bitboard king_attack_table[NUM_SQUARE];

Bitboard soldier_mask(Square square) {
    Bitboard attacks = BB_EMPTY;
    Bitboard bitboard = bb_from_sq(square);

    if (bitboard << DIR_NORT)
        attacks |= (bitboard << DIR_NORT);
    if (bitboard >> DIR_NORT)
        attacks |= (bitboard >> DIR_NORT);
    if ((bitboard << DIR_EAST) & BB_NOT_A_FILE)
        attacks |= (bitboard << DIR_EAST);
    if ((bitboard >> DIR_EAST) & BB_NOT_H_FILE)
        attacks |= (bitboard >> DIR_EAST);

    return attacks;
}

Bitboard soldier_attacks(Square square) { return soldier_attack_table[square]; }

Bitboard king_mask(Square square) {
    Bitboard attacks = BB_EMPTY;
    Bitboard bitboard = bb_from_sq(square);

    if (bitboard >> 8)
        attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & BB_NOT_H_FILE)
        attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & BB_NOT_A_FILE)
        attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & BB_NOT_H_FILE)
        attacks |= (bitboard >> 1);

    if (bitboard << 8)
        attacks |= (bitboard << 8);
    if ((bitboard << 9) & BB_NOT_A_FILE)
        attacks |= (bitboard << 9);
    if ((bitboard << 7) & BB_NOT_H_FILE)
        attacks |= (bitboard << 7);
    if ((bitboard << 1) & BB_NOT_A_FILE)
        attacks |= (bitboard << 1);

    return attacks;
}

Bitboard king_attacks(Square square) { return king_attack_table[square]; };

Bitboard general_attacks_slow(Square sq, Bitboard blockers) {
    Bitboard moves = BB_EMPTY;

    int start_rank = sq_rank(sq);
    int start_file = sq_file(sq);

    for (int rank = start_rank + 1; rank <= RANK_7; rank++) {
        moves |= bb_from_sq(sq_get(start_file, rank));

        if (blockers & bb_from_sq(sq_get(start_file, rank))) {
            break;
        }
    }

    for (int rank = start_rank - 1; rank >= RANK_1; rank--) {
        moves |= bb_from_sq(sq_get(start_file, rank));

        if (blockers & bb_from_sq(sq_get(start_file, rank))) {
            break;
        }
    }

    for (int file = start_file + 1; file <= FILE_H; file++) {
        moves |= bb_from_sq(sq_get(file, start_rank));

        if (blockers & bb_from_sq(sq_get(file, start_rank))) {
            break;
        }
    }

    for (int file = start_file - 1; file >= FILE_A; file--) {
        moves |= bb_from_sq(sq_get(file, start_rank));

        if (blockers & bb_from_sq(sq_get(file, start_rank))) {
            break;
        }
    }

    return moves;
};

Bitboard general_mask(Square sq) {
    Bitboard moves = BB_EMPTY;

    int start_rank = sq_rank(sq);
    int start_file = sq_file(sq);

    for (int rank = start_rank + 1; rank <= RANK_7; rank++) {
        moves |= bb_from_sq(sq_get(start_file, rank));
    }

    for (int rank = start_rank - 1; rank >= RANK_2; rank--) {
        moves |= bb_from_sq(sq_get(start_file, rank));
    }

    for (int file = start_file + 1; file <= FILE_G; file++) {
        moves |= bb_from_sq(sq_get(file, start_rank));
    }

    for (int file = start_file - 1; file >= FILE_B; file--) {
        moves |= bb_from_sq(sq_get(file, start_rank));
    }
    return moves;
};

size_t magic_index(Magic *entry, Bitboard blockers) {
    blockers &= entry->mask;
    blockers *= entry->magic;
    blockers >>= entry->offset;

    return blockers;
}

Bitboard general_attacks(Square sq, Bitboard blockers) {
    Magic *entry = &magics[sq];
    return entry->ptr[magic_index(entry, blockers)];
}

bool try_magic(Square sq, Magic *entry) {
    Bitboard blockers = BB_EMPTY;

    memset(entry->ptr, 0, (1 << (64 - entry->offset)) * sizeof(Bitboard));

    for (;;) {
        Bitboard moves = general_attacks_slow(sq, blockers);
        Bitboard *table_entry = &entry->ptr[magic_index(entry, blockers)];

        if (*table_entry == BB_EMPTY) {
            *table_entry = moves;
        } else if (*table_entry != moves) {
            return false;
        }

        // Carry-Rippler's trick
        blockers = (blockers - entry->mask) & entry->mask;

        if (blockers == BB_EMPTY) {
            return true;
        }
    }
}

void init_leaper_attacks() {
    for (Square sq = SQ_A1; sq <= SQ_H7; sq++) {
        soldier_attack_table[sq] = soldier_mask(sq);
        king_attack_table[sq] = king_mask(sq);
    }
}

void init_slider_attacks() {
    Bitboard *ptr = general_attack_table;
    for (Square sq = SQ_A1; sq <= SQ_H7; sq++) {
        Bitboard mask = general_mask(sq);
        uint8_t index_bits = bb_popcnt(mask);

        for (;;) {
            uint64_t magic = random_u64() & random_u64() & random_u64();

            if (bb_popcnt((mask * magic) & 0xFF00000000000000ULL) < 6) {
                continue;
            }

            Magic entry = {
                    .mask = mask,
                    .magic = magic,
                    .offset = 64 - index_bits,
                    .ptr = ptr,
            };

            if (try_magic(sq, &entry)) {
                magics[sq] = entry;
                ptr += (1 << (64 - entry.offset));
                break;
            }
        }
    }
}

typedef struct {
    int value;
    Square from;
    Square to;
    Bitboard past_captures;
} CaptureSequence;

// Helper function to check if a destination square is valid
static inline bool is_valid_destination(Square to, Square target, Bitboard combined) {
    Bitboard anti_wrap_mask = BB_FULL;
    if (sq_file(target) == FILE_A)
        anti_wrap_mask &= BB_NOT_H_FILE;
    else if (sq_file(target) == FILE_B)
        anti_wrap_mask &= BB_NOT_A_FILE;

    return !bb_is_empty(bb_from_sq(to) & anti_wrap_mask & ~combined);
}

// Helper function to calculate capture value
static inline int calculate_capture_value(Position *pos, Square target, Color color, int current_value) {
    int new_value = current_value;
    for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
        if (pos->pieces[1 - color][piece] & bb_from_sq(target)) {
            new_value += (int) piece + 1;
            break;
        }
    }
    return new_value;
}

// Helper function to store sequence in capture list if no more captures possible
static inline void store_final_sequence(CaptureSequence sequence, CaptureSequence capture_list[256]) {
    for (int i = 0; i < 256; i++) {
        if (capture_list[i].value < sequence.value) {
            capture_list[i] = sequence;
            return;
        }
    }
}

// Common setup logic for all capture functions
static inline void setup_capture_search(Position *pos, Bitboard *my_pieces, Bitboard *enemy_pieces,
                                        Bitboard *combined) {
    Color color = pos->side_to_move;

    *my_pieces = pos->pieces[color][PIECE_SOLDIER] | pos->pieces[color][PIECE_GENERAL] | pos->pieces[color][PIECE_KING];

    *enemy_pieces = pos->pieces[1 - color][PIECE_SOLDIER] | pos->pieces[1 - color][PIECE_GENERAL] |
                    pos->pieces[1 - color][PIECE_KING];

    *combined = *my_pieces | *enemy_pieces;
}

void find_king_captures(Position *pos, CaptureSequence sequence, CaptureSequence capture_list[256]) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = king_attacks(from) & BB_USED;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.past_captures & BB_NOT_EDGE & BB_USED;

    // If no capture
    if (potential_captures_it == BB_EMPTY) {
        store_final_sequence(sequence, capture_list);
        return;
    }

    while (potential_captures_it) {
        Square target = bb_it_next(&potential_captures_it);
        Direction direction = (int) target - (int) from;
        Square to = target + direction;

        // Check if destination is valid
        if (!is_valid_destination(to, target, combined)) {
            continue;
        }

        int new_value = calculate_capture_value(pos, target, color, sequence.value);

        CaptureSequence new_sequence = {.value = new_value,
                                        .from = sequence.from,
                                        .to = to,
                                        .past_captures = sequence.past_captures | bb_from_sq(target)};

        find_king_captures(pos, new_sequence, capture_list);
    }
}

void find_general_captures(Position *pos, CaptureSequence sequence, CaptureSequence capture_list[256]) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = general_attacks(from, enemy_pieces);

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.past_captures & BB_USED;

    // If no capture
    if (potential_captures_it == BB_EMPTY) {
        store_final_sequence(sequence, capture_list);
        return;
    }

    while (potential_captures_it) {
        Square target = bb_it_next(&potential_captures_it);

        int file_diff = (int) sq_file(target) - (int) sq_file(from);
        int rank_diff = (int) sq_rank(target) - (int) sq_rank(from);

        Direction direction;

        if (file_diff > 0 && rank_diff == 0)
            direction = DIR_EAST;
        else if (file_diff < 0 && rank_diff == 0)
            direction = DIR_WEST;
        else if (file_diff == 0 && rank_diff > 0)
            direction = DIR_NORT;
        else if (file_diff == 0 && rank_diff < 0)
            direction = DIR_SOUT;
        else if (file_diff > 0 && rank_diff > 0)
            direction = DIR_NOEA;
        else if (file_diff > 0 && rank_diff < 0)
            direction = DIR_SOWE;
        else if (file_diff < 0 && rank_diff > 0)
            direction = DIR_SOEA;
        else
            direction = DIR_NOWE;

        Square to = target + direction;

        // For general, check multiple landing squares
        while (is_valid_destination(to, target, combined)) {
            int new_value = calculate_capture_value(pos, target, color, sequence.value);

            CaptureSequence new_sequence = {.value = new_value,
                                            .from = sequence.from,
                                            .to = to,
                                            .past_captures = sequence.past_captures | bb_from_sq(target)};

            find_general_captures(pos, new_sequence, capture_list);

            to += direction;
        }
    }
}

void find_soldier_captures(Position *pos, CaptureSequence sequence, CaptureSequence capture_list[256]) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = soldier_attacks(from) & BB_USED;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.past_captures & BB_NOT_EDGE & BB_USED;

    // If no capture
    if (potential_captures_it == BB_EMPTY) {
        store_final_sequence(sequence, capture_list);
        return;
    }

    while (potential_captures_it) {
        Square target = bb_it_next(&potential_captures_it);
        Direction direction = (int) target - (int) from;
        Square to = target + direction;

        // Check if destination is valid
        if (!is_valid_destination(to, target, combined)) {
            continue;
        }

        int new_value = calculate_capture_value(pos, target, color, sequence.value);

        CaptureSequence new_sequence = {.value = new_value,
                                        .from = sequence.from,
                                        .to = to,
                                        .past_captures = sequence.past_captures | bb_from_sq(target)};

        find_soldier_captures(pos, new_sequence, capture_list);
    }
}

MoveList legal_moves(Position *pos) {
    // Do all the captures
    // pos->
    MoveList move_list = {0};
    CaptureSequence capture_sequence_list[256] = {0};

    Color color = pos->side_to_move;

    Bitboard my_pieces =
            pos->pieces[color][PIECE_SOLDIER] | pos->pieces[color][PIECE_GENERAL] | pos->pieces[color][PIECE_KING];

    Bitboard enemy_pieces = pos->pieces[1 - color][PIECE_SOLDIER] | pos->pieces[1 - color][PIECE_GENERAL] |
                            pos->pieces[1 - color][PIECE_KING];

    Bitboard combined = my_pieces | enemy_pieces;

    // Check for captures

    {
        Bitboard kings_it = pos->pieces[color][PIECE_KING] & BB_USED;
        while (kings_it) {
            Square from = bb_it_next(&kings_it);

            CaptureSequence sequence = {
                    .value = 0,
                    .past_captures = BB_EMPTY,
                    .from = from,
                    .to = from,
            };

            find_king_captures(pos, sequence, capture_sequence_list);
        }
    }

    {
        Bitboard general_it = pos->pieces[color][PIECE_GENERAL] & BB_USED;
        while (general_it) {
            Square from = bb_it_next(&general_it);

            CaptureSequence sequence = {
                    .value = 0,
                    .past_captures = BB_EMPTY,
                    .from = from,
                    .to = from,
            };

            find_general_captures(pos, sequence, capture_sequence_list);
        }
    }

    {
        Bitboard soldier_it = pos->pieces[color][PIECE_GENERAL] & BB_USED;
        while (soldier_it) {
            Square from = bb_it_next(&soldier_it);

            CaptureSequence sequence = {
                    .value = 0,
                    .past_captures = BB_EMPTY,
                    .from = from,
                    .to = from,
            };

            find_soldier_captures(pos, sequence, capture_sequence_list);
        }
    }

    // Find the longest capture sequence(s)
    {
        int max = 0;
        for (int i = 0; i < 256; i++) {
            CaptureSequence sequence = capture_sequence_list[i];

            if (sequence.value == 0)
                break;

            if (sequence.value > max) {
                max = capture_sequence_list[i].value;
            }
        }

        // Append legal capture sequences to legal moves

        for (int i = 0; i < 256; i++) {
            CaptureSequence sequence = capture_sequence_list[i];

            if (sequence.value == 0)
                break;

            if (sequence.value == max) {
                move_list.moves[move_list.size++] = (Move) {
                        .from = sequence.from,
                        .to = sequence.to,
                        .captured = sequence.past_captures,
                };
            }
        }

        if (max > 0) {
            return move_list;
        }
    }

    // No captures were found, looking for normal moves

    // FIXME: Can we guarantee that the superior bit are unused
    Bitboard kings_it = pos->pieces[color][PIECE_KING] & BB_USED;
    while (kings_it) {
        Square from = bb_it_next(&kings_it);
        Bitboard attacks_it = king_attacks(from) & ~combined & BB_USED;

        while (attacks_it) {
            Square to = bb_it_next(&attacks_it);
            move_list.moves[move_list.size++] = (Move) {
                    .from = from,
                    .to = to,
                    .captured = BB_EMPTY,
            };
        }
    }

    Bitboard generals_it = pos->pieces[color][PIECE_GENERAL] & BB_USED;
    while (generals_it) {
        Square from = bb_it_next(&generals_it);
        Bitboard attacks_it = general_attacks(from, combined) & ~combined & BB_USED;

        while (attacks_it) {
            Square to = bb_it_next(&attacks_it);
            move_list.moves[move_list.size++] = (Move) {
                    .from = from,
                    .to = to,
                    .captured = BB_EMPTY,
            };
        }
    }

    Bitboard soldier_it = pos->pieces[color][PIECE_SOLDIER] & BB_USED;
    while (soldier_it) {
        Square from = bb_it_next(&soldier_it);
        Bitboard attacks_it = soldier_attacks(from) & ~combined & BB_USED;

        while (attacks_it) {
            Square to = bb_it_next(&attacks_it);
            move_list.moves[move_list.size++] = (Move) {
                    .from = from,
                    .to = to,
                    .captured = BB_EMPTY,
            };
        }
    }

    return move_list;
}

void init() {
    init_leaper_attacks();
    init_slider_attacks();

    Position pos = (Position) {
            .pieces = {0},
            .ply = 15,
            .side_to_move = COLOR_WHITE,
    };

    pos.pieces[COLOR_WHITE][PIECE_GENERAL] = bb_from_sq(SQ_E4);
    pos.pieces[COLOR_BLACK][PIECE_SOLDIER] = bb_from_sq(SQ_E5);
    pos.pieces[COLOR_BLACK][PIECE_SOLDIER] |= bb_from_sq(SQ_D6);
    pos.pieces[COLOR_BLACK][PIECE_SOLDIER] |= bb_from_sq(SQ_A3);
    pos.pieces[COLOR_BLACK][PIECE_SOLDIER] |= bb_from_sq(SQ_G2);

    position_print(&pos);

    MoveList move_list = legal_moves(&pos);
    printf("Move list size: %d\n", move_list.size);

    for (int i = 0; i < move_list.size; i++) {
        char from[3];
        char to[3];

        sq_to_string(move_list.moves[i].from, from);
        sq_to_string(move_list.moves[i].to, to);

        printf("Move:\n");
        printf("  from: %s\n", from);
        printf("  to: %s\n", to);
        printf("  captures:\n");
        bb_print(move_list.moves[i].captured);
    }

    // Bitboard attacks = general_attacks(SQ_E4, BB_EMPTY);
    // bb_print(attacks);
    //
    // attacks = king_attacks(SQ_E4);
    // bb_print(attacks);
    //
    // attacks = king_attacks(SQ_H7);
    // bb_print(attacks);
}


void act(const char *position, double time_remaining) {}
