import fenix
import fen_utils
from fenix import FenixAction

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


def perft(state: fenix.FenixState, depth):
    if depth == 0 or state.is_terminal():
        return 1

    num_nodes = 0
    for action in frozenset(state.actions()):
        src = chr(ord("a") + action.start[1]) + chr(ord("7") - action.start[0])
        dst = chr(ord("a") + action.end[1]) + chr(ord("7") - action.end[0])

        move_num_nodes = perft_aux(state.result(action), depth - 1)

        if src == "d4" and dst == "d3":
            print(action)

        print(f"{src}-{dst}: {move_num_nodes}")
        num_nodes += move_num_nodes

    return num_nodes


def perft_aux(state: fenix.FenixState, depth):
    if depth == 0 or state.is_terminal():
        return 1

    num_nodes = 0
    for action in frozenset(state.actions()):
        num_nodes += perft_aux(state.result(action), depth - 1)

    return num_nodes

if __name__ == "__main__":
    state = fen_utils.fen_to_state("1G1SK3/G7/S1S4k/SSSS2g1/SS2ssgs/S2ssgs1/2ssssss 22 w 10")
    # state = state.result(FenixAction(start=(3, 3), end=(4, 3), removed=frozenset()))
    print(state_to_fen(state))
    print(perft(state, 4))
