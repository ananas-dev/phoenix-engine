import re

from fenix import FenixState

def fen_to_state(fen):
    """
    Convert a FEN string to a FenixState object.

    Args:
        fen (str): The FEN string representing the game state.

    Returns:
        FenixState: A FenixState object reconstructed from the FEN string.
    """
    # Split the FEN string into components
    parts = fen.split()
    board_fen, turn, current_player, creation_flags = parts

    # Determine board dimensions
    rows = board_fen.split("/")
    num_rows = len(rows)
    num_cols = max(sum(int(c) if c.isdigit() else 1 for c in row) for row in rows)

    # Initialize empty board state
    pieces = {}

    # Map FEN characters to piece values
    piece_symbols = {
        'S': 1,  's': -1,  # Soldiers
        'G': 2,  'g': -2,  # Generals
        'K': 3,  'k': -3   # Kings
    }

    # Parse the board
    for r, row in enumerate(rows):
        col = 0
        for char in row:
            if char.isdigit():
                col += int(char)  # Skip empty squares
            else:
                pieces[(r, col)] = piece_symbols[char]
                col += 1

    # Parse other game state components
    turn = int(turn)
    current_player = 1 if current_player == "w" else -1
    can_create_general = creation_flags[0] == "1"
    can_create_king = creation_flags[1] == "1"

    state = FenixState()
    state.pieces = pieces
    state.turn = turn
    state.current_player = current_player
    state.can_create_general = can_create_general
    state.can_create_king = can_create_king

    return state
