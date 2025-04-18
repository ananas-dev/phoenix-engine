from agent import Agent
import ctypes
import random
from fenix import FenixAction

def bb_it_next(b):
    if b == 0:
        raise ValueError("Bitboard is empty")

    s = (b & -b).bit_length() - 1
    b &= b - 1
    return s, b

def state_to_fen(state):
    """
    Convert a FenixState object to a FEN string.

    Args:
        state: FenixState object representing the current game state

    Returns:
        A FEN string representing the position
    """
    # Get board dimensions
    rows, cols = state.dim

    # Map piece values to FEN characters
    piece_symbols = {
        1: 'S',  # White soldier
        -1: 's',  # Black soldier
        2: 'G',  # White general
        -2: 'g',  # Black general
        3: 'K',  # White king
        -3: 'k',  # Black king
    }

    # Build the board representation
    fen_rows = []
    for row in range(rows):
        empty_count = 0
        row_str = ""

        for col in range(cols):
            piece = state.pieces.get((row, col))

            if piece is None:
                empty_count += 1
            else:
                if empty_count > 0:
                    row_str += str(empty_count)
                    empty_count = 0
                row_str += piece_symbols[piece]

        if empty_count > 0:
            row_str += str(empty_count)

        fen_rows.append(row_str)

    # Join the rows with '/'
    board_fen = "/".join(fen_rows)

    # Add the turn count
    board_fen += f" {state.turn}"

    # Add side to move (w for 1, b for -1)
    board_fen += " w" if state.current_player == 1 else " b"

    # Add creation flags
    creation_flags = ""
    creation_flags += "1" if state.can_create_general else "0"
    creation_flags += "1" if state.can_create_king else "0"
    board_fen += f" {creation_flags}"

    return board_fen

class Move(ctypes.Structure):
    _fields_ = [
        ("captures", ctypes.c_uint64),
        ("src", ctypes.c_uint8),
        ("dst", ctypes.c_uint8),
    ]

class State(ctypes.Structure):
    pass

StatePtr = ctypes.POINTER(State)

class IAAgent(Agent):
    def __init__(self, player):
        super().__init__(player)
        self.lib = ctypes.CDLL("../cmake-build-release/libclippy.so")
        self.lib.act.argtypes = [StatePtr, ctypes.c_char_p, ctypes.c_double]
        self.lib.act.restype = Move
        self.lib.init.restype = StatePtr
        self.lib.destroy.argtypes = [StatePtr]
        self.state = self.lib.init()

    def act(self, state, remaining_time):
        fen = state_to_fen(state)
        print(fen)

        move = self.lib.act(self.state, fen.encode("utf-8"), remaining_time)

        src_file = move.src & 7
        src_rank = move.src >> 3
        dst_file = move.dst & 7
        dst_rank = move.dst >> 3

        deleted = set()
        captures_it = move.captures
        while captures_it:
            i, captures_it = bb_it_next(captures_it)
            deleted_file = i & 7
            deleted_rank = i >> 3

            deleted.add((6 - deleted_rank, deleted_file))

        action = FenixAction((6 - src_rank, src_file), (6 - dst_rank, dst_file), frozenset(deleted))
        return action

    def __del__(self):
        self.lib.destroy(self.state)
