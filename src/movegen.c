#include "movegen.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static uint64_t random_u64() {
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

typedef struct {
    int size;
    CaptureSequence sequences[256];
} CaptureSequenceList;

// Helper function to check if a destination square is valid
static inline bool is_valid_destination(Square dest, Direction direction, Bitboard combined) {
    Bitboard anti_wrap_mask = BB_FULL;

    if (direction == DIR_EAST || direction == DIR_NOEA || direction == DIR_SOEA) {
        anti_wrap_mask &= BB_NOT_A_FILE;
    } else if (direction == DIR_WEST || direction == DIR_NOWE || direction == DIR_SOWE)  {
        anti_wrap_mask &= BB_NOT_H_FILE;
    }

    return !bb_is_empty(bb_from_sq(dest) & anti_wrap_mask & ~combined);
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
static inline void store_final_sequence(CaptureSequence sequence, CaptureSequenceList *capture_list) {
    // Skip empty sequences
    if (sequence.from == sequence.to) {
        return;
    }

    for (int i = 0; i < capture_list->size; i++) {
        if (capture_list->sequences[i].value < sequence.value) {
            capture_list->sequences[i] = sequence;
            return;
        }
    }

    // If no captures were overwritten
    capture_list->sequences[capture_list->size++] = sequence;
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

void find_king_captures(Position *pos, CaptureSequence sequence, CaptureSequenceList *capture_list) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = king_attacks(from) & BB_USED;

    bool found_capture = false;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.past_captures & BB_USED;

    while (potential_captures_it) {
        Square target = bb_it_next(&potential_captures_it);
        Direction direction = (int) target - (int) from;
        Square to = target + direction;

        // Check if destination is valid
        if (!is_valid_destination(to, direction, combined)) {
            continue;
        }

        found_capture = true;

        int new_value = calculate_capture_value(pos, target, color, sequence.value);

        CaptureSequence new_sequence = {
            .value = new_value,
            .from = sequence.from,
            .to = to,
            .past_captures = sequence.past_captures | bb_from_sq(target)
        };

        find_king_captures(pos, new_sequence, capture_list);
    }

    // If no capture
    if (!found_capture) {
        store_final_sequence(sequence, capture_list);
    }
}

void find_general_captures(Position *pos, CaptureSequence sequence, CaptureSequenceList *capture_list) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = general_attacks(from, combined);

    bool found_capture = false;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.past_captures & BB_USED;

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
        while (is_valid_destination(to, direction, combined)) {
            found_capture = true;

            int new_value = calculate_capture_value(pos, target, color, sequence.value);

            CaptureSequence new_sequence = {
                .value = new_value,
                .from = sequence.from,
                .to = to,
                .past_captures = sequence.past_captures | bb_from_sq(target)
            };

            find_general_captures(pos, new_sequence, capture_list);

            to += direction;
        }
    }

    // If no capture
    if (!found_capture) {
        store_final_sequence(sequence, capture_list);
    }
}

void find_soldier_captures(Position *pos, CaptureSequence sequence, CaptureSequenceList *capture_list) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = soldier_attacks(from) & BB_USED;

    bool found_capture = false;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.past_captures & BB_USED;

    while (potential_captures_it) {
        Square target = bb_it_next(&potential_captures_it);
        Direction direction = (int) target - (int) from;
        Square to = target + direction;

        // Check if destination is valid
        if (!is_valid_destination(to, direction, combined)) {
            continue;
        }

        found_capture = true;

        int new_value = calculate_capture_value(pos, target, color, sequence.value);

        CaptureSequence new_sequence = {
            .value = new_value,
            .from = sequence.from,
            .to = to,
            .past_captures = sequence.past_captures | bb_from_sq(target)
        };

        find_soldier_captures(pos, new_sequence, capture_list);
    }

    // If no capture
    if (!found_capture) {
        store_final_sequence(sequence, capture_list);
    }
}

void append_stacks(Position *pos, MoveList *move_list, Bitboard stack_targets) {
    Color color = pos->side_to_move;
    Bitboard soldier_it = pos->pieces[color][PIECE_SOLDIER] & BB_USED;

    while (soldier_it) {
        Square from = bb_it_next(&soldier_it);

        Bitboard stacks_it = soldier_attacks(from) & stack_targets & BB_USED;

        while (stacks_it) {
            Square to = bb_it_next(&stacks_it);
            move_list->moves[move_list->size++] = (Move){
                .from = from,
                .to = to,
                .captured = BB_EMPTY,
            };
        }
    }
}

void legal_moves(Position *pos, MoveList *move_list) {
    CaptureSequenceList capture_sequence_list = {0};

    Color color = pos->side_to_move;

    Bitboard my_pieces =
            pos->pieces[color][PIECE_SOLDIER] | pos->pieces[color][PIECE_GENERAL] | pos->pieces[color][PIECE_KING];

    Bitboard enemy_pieces = pos->pieces[1 - color][PIECE_SOLDIER] | pos->pieces[1 - color][PIECE_GENERAL] |
                            pos->pieces[1 - color][PIECE_KING];

    Bitboard combined = my_pieces | enemy_pieces;

    // Setup phase
    if (pos->ply < 10) {
        Bitboard stacks_target;

        // Only allow 1 king and 3 generals
        if (bb_popcnt(pos->pieces[color][PIECE_KING]) == 1) {
            stacks_target = pos->pieces[color][PIECE_SOLDIER];
        } else if (bb_popcnt(pos->pieces[color][PIECE_GENERAL]) == 4) {
            stacks_target = pos->pieces[color][PIECE_GENERAL];
        } else {
            stacks_target = pos->pieces[color][PIECE_SOLDIER] | pos->pieces[color][PIECE_GENERAL];
        }

        append_stacks(pos, move_list, stacks_target);
        return;
    }

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

            find_king_captures(pos, sequence, &capture_sequence_list);
        }
    } {
        Bitboard general_it = pos->pieces[color][PIECE_GENERAL] & BB_USED;
        while (general_it) {
            Square from = bb_it_next(&general_it);

            CaptureSequence sequence = {
                .value = 0,
                .past_captures = BB_EMPTY,
                .from = from,
                .to = from,
            };

            find_general_captures(pos, sequence, &capture_sequence_list);
        }
    } {
        Bitboard soldier_it = pos->pieces[color][PIECE_SOLDIER] & BB_USED;
        while (soldier_it) {
            Square from = bb_it_next(&soldier_it);

            CaptureSequence sequence = {
                .value = 0,
                .past_captures = BB_EMPTY,
                .from = from,
                .to = from,
            };

            find_soldier_captures(pos, sequence, &capture_sequence_list);
        }
    }

    // Find the longest capture sequence(s)
    {
        int max = 0;
        for (int i = 0; i < capture_sequence_list.size; i++) {
            CaptureSequence sequence = capture_sequence_list.sequences[i];

            if (sequence.value > max) {
                max = sequence.value;
            }
        }

        // Append legal capture sequences to legal moves

        for (int i = 0; i < capture_sequence_list.size; i++) {
            CaptureSequence sequence = capture_sequence_list.sequences[i];

            if (sequence.value == max) {
                move_list->moves[move_list->size++] = (Move){
                    .from = sequence.from,
                    .to = sequence.to,
                    .captured = sequence.past_captures,
                };
            }
        }

        if (max > 0) {
            return;
        }
    }

    // No captures were found, looking for stack moves
    if (pos->can_create_general) {
        Bitboard stacks_target = pos->pieces[color][PIECE_SOLDIER];
        append_stacks(pos, move_list, stacks_target);
    }

    if (pos->can_create_king) {
        Bitboard stacks_target = pos->pieces[color][PIECE_GENERAL];
        append_stacks(pos, move_list, stacks_target);
    }

    // Normal moves
    // FIXME: Can we guarantee that the superior bit are unused
    Bitboard kings_it = pos->pieces[color][PIECE_KING] & BB_USED;
    while (kings_it) {
        Square from = bb_it_next(&kings_it);
        Bitboard attacks_it = king_attacks(from) & ~combined & BB_USED;

        while (attacks_it) {
            Square to = bb_it_next(&attacks_it);
            move_list->moves[move_list->size++] = (Move){
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
            move_list->moves[move_list->size++] = (Move){
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
            move_list->moves[move_list->size++] = (Move){
                .from = from,
                .to = to,
                .captured = BB_EMPTY,
            };
        }
    }
}

void movegen_init() {
    init_leaper_attacks();
    init_slider_attacks();
}
