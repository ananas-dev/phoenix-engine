import os


def get_os_specific_library():
    # Detect OS and architecture
    system = platform.system().lower()
    machine = platform.machine().lower()

    # Map platform.machine() to common arch names
    arch_map = {
        "x86_64": "x86_64",
        "amd64": "x86_64",
        "aarch64": "aarch64",
        "arm64": "aarch64",
    }
    arch = arch_map.get(machine, machine)  # Default to raw value if not mapped

    # Construct the library filename
    lib_prefix = {
        "linux": "libclippy-linux",
        "windows": "clippy-windows",
        "darwin": "libclippy-macos",
    }.get(system, None)

    if system == "linux" and arch == "x86_64":
        code = CODE_LINUX_X86_64
    elif system == "linux" and arch == "aarch64":
        code = CODE_LINUX_AARCH64
    elif system == "windows" and arch == "x86_64":
        code = CODE_WINDOWS_X86_64
    elif system == "windows" and arch == "aarch64":
        code = CODE_WINDOWS_AARCH64
    elif system == "darwin" and arch == "x86_64":
        code = CODE_MACOS_X86_64
    elif system == "darwin" and arch == "aarch64":
        code = CODE_MACOS_AARCH64
    else:
        raise RuntimeError(f"Unsupported OS: {system}")

    return code


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

    board_fen += f" {state.boring_turn}"

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
        ("found_mate", ctypes.c_bool),
    ]

class Context(ctypes.Structure):
    pass

ContextPtr = ctypes.POINTER(Context)

class State(ctypes.Structure):
    pass

StatePtr = ctypes.POINTER(State)

class ClippyAgent(Agent):
    def __init__(self, player, debug=False):
        super().__init__(player)
        dll_data_zip = base64.b64decode(get_os_specific_library())
        dll_data = gzip.decompress(dll_data_zip)

        fd, temp_path = tempfile.mkstemp(suffix='.so')
        self.__temp_path = temp_path

        # Write the decoded data to the temporary file
        with os.fdopen(fd, 'wb') as temp_file:
            temp_file.write(dll_data)

        self.lib = ctypes.CDLL(temp_path)

        # Act function
        self.lib.act.argtypes = [StatePtr, ctypes.c_char_p, ctypes.c_double, ctypes.c_bool]
        self.lib.act.restype = Move

        # Init function
        self.lib.init_context.argtypes = [ctypes.c_bool]
        self.lib.init_context.restype = ContextPtr

        # Set weight function
        self.lib.set_weights.argtypes = [ContextPtr, ctypes.POINTER(ctypes.c_int), ctypes.c_int]

        # Destroy function
        self.lib.destroy_context.argtypes = [ContextPtr]
        self.lib.destroy_state.argtypes = [StatePtr]

        # New game function
        self.lib.new_game.argtypes = [ContextPtr, StatePtr]
        self.lib.new_game.restype = StatePtr

        self.__found_mate = False
        self.__state = None
        self.__ctx = self.lib.init_context(debug)
        self.set_weights(WEIGHTS)

    def new_game(self):
        self.__found_mate = False
        self.__state = self.lib.new_game(self.__ctx, self.__state)

    def act(self, state, remaining_time):
        if state.turn < 2:
            self.new_game()

        fen = state_to_fen(state)

        is_irreversible = len(state.history_boring_turn_hash) == 0

        move = self.lib.act(self.__state, fen.encode("utf-8"), remaining_time, is_irreversible)

        src_file = move.src & 7
        src_rank = move.src >> 3
        dst_file = move.dst & 7
        dst_rank = move.dst >> 3

        self.__found_mate = move.found_mate

        deleted = set()
        captures_it = move.captures
        while captures_it:
            i, captures_it = bb_it_next(captures_it)
            deleted_file = i & 7
            deleted_rank = i >> 3

            deleted.add((6 - deleted_rank, deleted_file))

        action = FenixAction((6 - src_rank, src_file), (6 - dst_rank, dst_file), frozenset(deleted))
        return action

    def found_mate(self):
        return self.__found_mate

    def set_weights(self, weights: list[int]):
        Weights = ctypes.c_int * len(weights)
        arr = Weights(*weights)
        self.lib.set_weights(self.__ctx, arr, len(weights))

    def __del__(self):
        if self.__state is not None:
            self.lib.destroy_state(self.__state)

        self.lib.destroy_context(self.__ctx)

        try:
            os.remove(self.__temp_path)
        except:
            print("Could not delete temp file !")

