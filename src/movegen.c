#include "movegen.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint64_t random_u64(void) {
    static uint64_t next = 1;

    next = next * 1103515245 + 12345;
    return next;
}

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

Bitboard soldier_attacks(State *state, Square square) { return state->soldier_attack_table[square]; }

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

Bitboard king_attacks(State *state, Square square) { return state->king_attack_table[square]; }

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
}

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
}

size_t magic_index(Magic *entry, Bitboard blockers) {
    blockers &= entry->mask;
    blockers *= entry->magic;
    blockers >>= entry->offset;

    return blockers;
}

Bitboard general_attacks(State *state, Square sq, Bitboard blockers) {
    Magic *entry = &state->magics[sq];
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

void init_leaper_attacks(State *state) {
    for (Square sq = SQ_A1; sq <= SQ_H7; sq++) {
        state->soldier_attack_table[sq] = soldier_mask(sq);
        state->king_attack_table[sq] = king_mask(sq);
    }
}

void init_slider_attacks(State *state) {
    Bitboard *ptr = state->general_attack_table;
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
                state->magics[sq] = entry;
                ptr += (1 << (64 - entry.offset));
                break;
            }
        }
    }
}

typedef struct {
    uint8_t max_value;
} CaptureInfo;

// Helper function to check if a destination square is valid
static inline bool is_valid_destination(Square dest, Direction direction, Bitboard combined) {
    Bitboard anti_wrap_mask = BB_FULL;

    if (direction == DIR_EAST || direction == DIR_NOEA || direction == DIR_SOEA) {
        anti_wrap_mask &= BB_NOT_A_FILE;
    } else if (direction == DIR_WEST || direction == DIR_NOWE || direction == DIR_SOWE) {
        anti_wrap_mask &= BB_NOT_H_FILE;
    }

    return !bb_is_empty(bb_from_sq(dest) & anti_wrap_mask & ~combined);
}

// Helper function to calculate capture value
static inline uint8_t calculate_capture_value(Position *pos, Square target, Color color, uint8_t current_value) {
    int new_value = current_value;
    for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
        if (pos->pieces[1 - color][piece] & bb_from_sq(target)) {
            new_value += (uint8_t) piece + 1;
            break;
        }
    }
    return new_value;
}

// Helper function to store sequence in capture list if no more captures possible
static inline void insert_sequence(Move sequence, uint8_t value, MoveList *capture_list, CaptureInfo *capture_info) {
    // Skip empty sequences
    if (sequence.from == sequence.to) {
        return;
    }

    if (capture_info->max_value == 0 || value > capture_info->max_value) {
        capture_info->max_value = value;
        capture_list->size = 1;
        capture_list->moves[0] = sequence;
    } else if (value == capture_info->max_value) {
        for (int i = 0; i < capture_list->size; i++) {
            Move other_sequence = capture_list->moves[i];

            if (sequence.from == other_sequence.from && sequence.to == other_sequence.to && sequence.captures ==
                other_sequence.captures) {
                return; // Avoid duplicates
            }
        }

        capture_list->moves[capture_list->size++] = sequence;
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

void find_king_captures(State *state, Position *pos, Move sequence, uint8_t value, MoveList *capture_list, CaptureInfo *capture_info) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = king_attacks(state, from) & BB_USED;

    bool found_capture = false;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.captures & BB_USED;

    while (potential_captures_it) {
        Square target = bb_it_next(&potential_captures_it);
        Direction direction = (int) target - (int) from;
        Square to = target + direction;

        // Check if destination is valid
        if (!is_valid_destination(to, direction, combined)) {
            continue;
        }

        found_capture = true;

        int new_value = calculate_capture_value(pos, target, color, value);

        Move new_sequence = {
            .from = sequence.from,
            .to = to,
            .captures = sequence.captures | bb_from_sq(target)
        };

        find_king_captures(state, pos, new_sequence, new_value, capture_list, capture_info);

    }

    // If no capture
    if (!found_capture) {
        insert_sequence(sequence, value, capture_list, capture_info);
    }
}

void find_general_captures(State *state, Position *pos, Move sequence, uint8_t value, MoveList *capture_list, CaptureInfo *capture_info) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = general_attacks(state, from, combined);

    bool found_capture = false;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.captures & BB_USED;

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

            int new_value = calculate_capture_value(pos, target, color, value);

            Move new_sequence = {
                .from = sequence.from,
                .to = to,
                .captures = sequence.captures | bb_from_sq(target)
            };

            find_general_captures(state, pos, new_sequence, new_value, capture_list, capture_info);

            to += direction;
        }
    }

    // If no capture
    if (!found_capture) {
        insert_sequence(sequence, value, capture_list, capture_info);
    }
}

void find_soldier_captures(State *state, Position *pos, Move sequence, uint8_t value, MoveList *capture_list, CaptureInfo *capture_info) {
    Bitboard my_pieces, enemy_pieces, combined;
    setup_capture_search(pos, &my_pieces, &enemy_pieces, &combined);

    Color color = pos->side_to_move;
    Square from = sequence.to;
    Bitboard attacks = soldier_attacks(state, from) & BB_USED;

    bool found_capture = false;

    // Add the capture mask of current sequence
    Bitboard potential_captures_it = attacks & enemy_pieces & ~sequence.captures & BB_USED;

    while (potential_captures_it) {
        Square target = bb_it_next(&potential_captures_it);
        Direction direction = (int) target - (int) from;
        Square to = target + direction;

        // Check if destination is valid
        if (!is_valid_destination(to, direction, combined)) {
            continue;
        }

        found_capture = true;

        int new_value = calculate_capture_value(pos, target, color, value);

        Move new_sequence = {
            .from = sequence.from,
            .to = to,
            .captures = sequence.captures | bb_from_sq(target)
        };

        find_soldier_captures(state, pos, new_sequence, new_value, capture_list, capture_info);
    }

    // If no capture
    if (!found_capture) {
        insert_sequence(sequence, value, capture_list, capture_info);
    }
}

void append_stacks(State *state, Position *pos, MoveList *move_list, Bitboard stack_targets) {
    Color color = pos->side_to_move;
    Bitboard soldier_it = pos->pieces[color][PIECE_SOLDIER] & BB_USED;

    while (soldier_it) {
        Square from = bb_it_next(&soldier_it);

        Bitboard stacks_it = soldier_attacks(state, from) & stack_targets & BB_USED;

        while (stacks_it) {
            Square to = bb_it_next(&stacks_it);
            move_list->moves[move_list->size++] = (Move){
                .from = from,
                .to = to,
                .captures = BB_EMPTY,
            };
        }
    }
}

void legal_moves(State *state, Position *pos, MoveList *move_list) {
    move_list->size = 0;

    Color color = pos->side_to_move;

    Bitboard my_pieces = pieces_by_color(pos, color);
    Bitboard enemy_pieces = pieces_by_color(pos, 1 - color);

    Bitboard combined = my_pieces | enemy_pieces;

    // Setup phase
    if (pos->ply < 10) {
        Bitboard stacks_target = BB_EMPTY;

        if (bb_popcnt(pos->pieces[color][PIECE_GENERAL]) < 4) {
            stacks_target |= pos->pieces[color][PIECE_SOLDIER];
        }

        if (bb_popcnt(pos->pieces[color][PIECE_KING]) == 0) {
            stacks_target |= pos->pieces[color][PIECE_GENERAL];
        }

        append_stacks(state, pos, move_list, stacks_target);
        return;
    }

    CaptureInfo capture_info = {0};

    // Check for captures
    {
        Bitboard kings_it = pos->pieces[color][PIECE_KING] & BB_USED;
        while (kings_it) {
            Square from = bb_it_next(&kings_it);

            Move sequence = {
                .captures = BB_EMPTY,
                .from = from,
                .to = from,
            };

            find_king_captures(state, pos, sequence, 0, move_list, &capture_info);
        }
    } {
        Bitboard general_it = pos->pieces[color][PIECE_GENERAL] & BB_USED;
        while (general_it) {
            Square from = bb_it_next(&general_it);

            Move sequence = {
                .captures = BB_EMPTY,
                .from = from,
                .to = from,
            };

            find_general_captures(state, pos, sequence, 0, move_list, &capture_info);
        }
    } {
        Bitboard soldier_it = pos->pieces[color][PIECE_SOLDIER] & BB_USED;
        while (soldier_it) {
            Square from = bb_it_next(&soldier_it);

             Move sequence = {
                .captures = BB_EMPTY,
                .from = from,
                .to = from,
            };

            find_soldier_captures(state, pos, sequence, 0, move_list, &capture_info);
        }
    }

    if (move_list->size > 0) {
        return;
    }

    // No captures were found, looking for stack moves
    if (pos->can_create_general) {
        Bitboard stacks_target = pos->pieces[color][PIECE_SOLDIER];
        append_stacks(state, pos, move_list, stacks_target);
    }

    if (pos->can_create_king) {
        Bitboard stacks_target = pos->pieces[color][PIECE_GENERAL];
        append_stacks(state, pos, move_list, stacks_target);
    }

    // Normal moves
    // FIXME: Can we guarantee that the superior bit are unused
    Bitboard kings_it = pos->pieces[color][PIECE_KING] & BB_USED;
    while (kings_it) {
        Square from = bb_it_next(&kings_it);
        Bitboard attacks_it = king_attacks(state, from) & ~combined & BB_USED;

        while (attacks_it) {
            Square to = bb_it_next(&attacks_it);
            move_list->moves[move_list->size++] = (Move){
                .from = from,
                .to = to,
                .captures = BB_EMPTY,
            };
        }
    }

    Bitboard generals_it = pos->pieces[color][PIECE_GENERAL] & BB_USED;
    while (generals_it) {
        Square from = bb_it_next(&generals_it);
        Bitboard attacks_it = general_attacks(state, from, combined) & ~combined & BB_USED;

        while (attacks_it) {
            Square to = bb_it_next(&attacks_it);
            move_list->moves[move_list->size++] = (Move){
                .from = from,
                .to = to,
                .captures = BB_EMPTY,
            };
        }
    }

    Bitboard soldier_it = pos->pieces[color][PIECE_SOLDIER] & BB_USED;
    while (soldier_it) {
        Square from = bb_it_next(&soldier_it);
        Bitboard attacks_it = soldier_attacks(state, from) & ~combined & BB_USED;

        while (attacks_it) {
            Square to = bb_it_next(&attacks_it);
            move_list->moves[move_list->size++] = (Move){
                .from = from,
                .to = to,
                .captures = BB_EMPTY,
            };
        }
    }
}

uint64_t perft_aux(State *state, Position *position, int depth) {
    if (depth == 0 || position_state(position)) {
        return 1ULL;
    }

    MoveList move_list = {0};
    legal_moves(state, position, &move_list);

    uint64_t num_nodes = 0;

    for (int i = 0; i < move_list.size; i++) {
        Position new_position = make_move(state, position, move_list.moves[i]);
        num_nodes += perft_aux(state, &new_position, depth - 1);
    }

    return num_nodes;
}


uint64_t perft(State *state, Position *position, int depth, bool debug) {
    if (depth == 0 || position_state(position)) {
        return 1ULL;
    }

    MoveList move_list = {0};
    legal_moves(state, position, &move_list);

    uint64_t num_nodes = 0;

    char from[3];
    char to[3];

    for (int i = 0; i < move_list.size; i++) {
        Position new_position = make_move(state, position, move_list.moves[i]);

        if (debug) {
            sq_to_string(move_list.moves[i].from, from);
            sq_to_string(move_list.moves[i].to, to);
        }

        uint64_t move_num_nodes = perft_aux(state, &new_position, depth - 1);

        if (debug) {
            printf("%s-%s: %lu\n", from, to, move_num_nodes);
        }

        num_nodes += move_num_nodes;
    }

    return num_nodes;
}

void movegen_init(State *state) {
    init_leaper_attacks(state);
    init_slider_attacks(state);
}
