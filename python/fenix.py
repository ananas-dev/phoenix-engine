from collections import namedtuple
from copy import deepcopy

FenixAction = namedtuple('FenixAction', ['start', 'end', 'removed'])
"""
FenixAction represents a move in the Fenix board game.

Attributes:
    start (tuple): The (row, column) position of the piece before the move.
    end (tuple): The (row, column) position of the piece after the move.
    removed (list of tuples): A list of (row, column) positions of pieces captured as a result of the move.
"""

class FenixState:
    """
    Represents the game state for the Fenix board game.

    Attributes:
        dim (tuple): The dimensions of the board (rows, columns).
        pieces (dict): A dictionary mapping (row, column) positions to piece values.
        turn (int): The current turn count.
        current_player (int): The player whose turn it is (1 or -1).
        can_create_general (bool): Flag indicating whether a general can be created.
        can_create_king (bool): Flag indicating whether a king can be created.
        precomputed_hash (int or None): Cached hash of the board state.
        history_boring_turn_hash (list): History of hashes for checking repetitions.
        boring_turn (int): Counter for turns without a capture (used for draw conditions).
    """
    def __init__(self):
        """
        Initializes a new FenixState with the starting configuration.
        """
        self.dim = (7, 8)

        self.pieces = dict()

        n_diag = 6
        for diag_i in range(0, n_diag):
            for diag_j in range(0, diag_i+1):
                self.pieces[(diag_i-diag_j, diag_j)] = 1
                self.pieces[(self.dim[0]-diag_i+diag_j-1, self.dim[1]-diag_j-1)] = -1

        self.turn = 0
        self.current_player = 1

        self.can_create_general = False
        self.can_create_king = False

        self.precomputed_hash = None

        self.history_boring_turn_hash = []
        self.boring_turn = 0

    def _is_inside(self, position):
        return 0 <= position[0] < self.dim[0] and 0 <= position[1] < self.dim[1]

    def _has_king(self, player):
        return 3*player in self.pieces.values()

    def _count_generals(self, player):
        return list(self.pieces.values()).count(2*player)

    def _has_piece(self, player):
        return len([p for p in self.pieces.values() if p * player > 0])

    def _setup_actions(self):
        actions = []
        for position, value in self.pieces.items():
            if value != self.current_player:
                continue
            for direction_i, direction_j in [(-1, 0), (0, -1), (1, 0), (0, 1)]:
                neighbor_position = (position[0] + direction_i, position[1] + direction_j)
                if neighbor_position not in self.pieces:
                    continue
                neighbor_type = self.pieces[neighbor_position]
                if ((neighbor_type == self.current_player and self._count_generals(self.current_player) < 4) or
                    (neighbor_type == 2*self.current_player and not self._has_king(self.current_player))):
                    actions.append(FenixAction(position, neighbor_position, frozenset()))
        return actions

    def _get_neighbors_soldier(self, start, end, removed, captured_units):
        neighbors = []
        for dir_i, dir_j in [(-1, 0), (0, -1), (1, 0), (0, 1)]:
            neighbor_position = (end[0]+dir_i, end[1]+dir_j)
            if not self._is_inside(neighbor_position):
                continue
            if neighbor_position in removed:
                continue
            next_neighbor_position = (neighbor_position[0]+dir_i, neighbor_position[1]+dir_j)
            if (self.pieces.get(neighbor_position, 0) * self.current_player < 0 and
                self._is_inside(next_neighbor_position) and
                next_neighbor_position not in self.pieces):
                neighbors.append((start, next_neighbor_position, removed.union([neighbor_position]), captured_units + abs(self.pieces[neighbor_position])))
                continue
            if captured_units == 0:
                if ((neighbor_position not in self.pieces) or
                    (self.can_create_general and self.pieces[neighbor_position] == self.current_player) or
                    (self.can_create_king and self.pieces[neighbor_position] == 2*self.current_player)):
                    neighbors.append((start, neighbor_position, removed, captured_units))
        return neighbors

    def _get_neighbors_general(self, start, end, removed, captured_units):
        neighbors = []
        for dir_i, dir_j in [(-1, 0), (0, -1), (1, 0), (0, 1)]:
            jumped = False
            jumped_piece = None
            for dist in range(1, 9):
                neighbor_position = (end[0]+(dist*dir_i), end[1]+(dist*dir_j))
                if not self._is_inside(neighbor_position):
                    break
                if (neighbor_position in self.pieces) and (self.pieces[neighbor_position] * self.current_player > 0):
                    break
                if (neighbor_position in removed):
                    break

                if not jumped:
                    if (neighbor_position not in self.pieces) and (captured_units == 0):
                        neighbors.append((start, neighbor_position, removed, captured_units))
                    elif (neighbor_position in self.pieces) and (self.pieces[neighbor_position] * self.current_player < 0):
                        jumped = True
                        jumped_piece = neighbor_position
                else:
                    if (neighbor_position not in self.pieces):
                        neighbors.append((start, neighbor_position, removed.union([jumped_piece]), captured_units + abs(self.pieces[jumped_piece])))
                    elif (neighbor_position in self.pieces) and (self.pieces[neighbor_position] * self.current_player < 0):
                        break
        return neighbors

    def _get_neighbors_king(self, start, end, removed, captured_units):
        neighbors = []
        for dir_i, dir_j in [(-1, 0), (0, -1), (1, 0), (0, 1), (-1, -1), (-1, 1), (1, 1), (1, -1)]:
            neighbor_position = (end[0]+dir_i, end[1]+dir_j)
            if not self._is_inside(neighbor_position):
                continue
            if neighbor_position in removed:
                continue
            next_neighbor_position = (neighbor_position[0]+dir_i, neighbor_position[1]+dir_j)
            if (self.pieces.get(neighbor_position, 0) * self.current_player < 0 and
                self._is_inside(next_neighbor_position) and
                next_neighbor_position not in self.pieces):
                neighbors.append((start, next_neighbor_position, removed.union([neighbor_position]), captured_units + abs(self.pieces[neighbor_position])))
                continue
            if captured_units == 0:
                if neighbor_position not in self.pieces:
                    neighbors.append((start, neighbor_position, removed, captured_units))
        return neighbors

    def _get_neighbors(self, start, end, removed, captured_units):
        start_type = abs(self.pieces[start])
        if start_type == 1:
            return self._get_neighbors_soldier(start, end, removed, captured_units)
        elif start_type == 2:
            return self._get_neighbors_general(start, end, removed, captured_units)
        elif start_type == 3:
            return self._get_neighbors_king(start, end, removed, captured_units)

    def _max_actions(self):
        action_container = self._ActionContainer()
        queue = []
        for position, value in self.pieces.items():
            if value * self.current_player < 0:
                continue
            queue.append((position, position, frozenset(), 0))
        while len(queue) > 0:
            current_start, current_end, current_removed, current_captured_units = queue.pop()
            for neighbor in self._get_neighbors(current_start, current_end, current_removed, current_captured_units):
                neighbor_start, neighbor_end, neighbor_removed, neighbor_captured_units = neighbor
                action_container.add(FenixAction(neighbor_start, neighbor_end, neighbor_removed), neighbor_captured_units)
                if current_captured_units < neighbor_captured_units:
                    queue.append(neighbor)
        return action_container.get_actions()

    def to_move(self):
        """
        Returns the player whose turn it is to move.

        Returns:
            int: The player whose turn it is (1 or -1).
        """
        return self.current_player

    def actions(self):
        """
        Returns the list of legal actions available in the current state.

        Returns:
            list of FenixAction: The available actions.
        """
        if self.turn < 10:
            return self._setup_actions()
        return self._max_actions()

    def result(self, action):
        """
        Returns the state that results from applying a given action.

        Args:
            action (FenixAction): The action to apply.

        Returns:
            FenixState: The new game state after the action.
        """
        state = deepcopy(self)

        start = action.start
        end = action.end
        removed = action.removed

        state.pieces[end] = state.pieces.get(end, 0) + state.pieces[start]
        state.pieces.pop(start)

        state.can_create_general = False
        state.can_create_king = False
        for removed_piece in removed:
            removed_piece_type = abs(state.pieces[removed_piece])
            if removed_piece_type == 2:
                state.can_create_general = True
            elif removed_piece_type == 3:
                state.can_create_king = True
            state.pieces.pop(removed_piece)

        state.turn += 1
        state.current_player = -state.current_player

        state.precomputed_hash = None

        if len(removed) > 0:
            state.boring_turn = 0
            state.history_boring_turn_hash = []
        elif state.turn > 10:
            state.boring_turn += 1
            state.history_boring_turn_hash.append(self._hash())

        return state

    def is_terminal(self):
        """
        Determines if the game has reached a terminal state.

        Returns:
            bool: True if the game is over, False otherwise.
        """
        if self.history_boring_turn_hash.count(self._hash()) >= 3:
            return True
        if self.boring_turn >= 50:
            return True
        if self.turn <= 10 and len(self.actions()) == 0:
            return True
        if self.turn > 10 and not self._has_king(-self.current_player):
            return True
        if not self._has_piece(1) or not self._has_piece(-1):
            return True
        return False

    def utility(self, player):
        """
        Computes the utility value for the given player.

        Args:
            player (int): The player for whom to calculate the utility (1 or -1).

        Returns:
            int: 1 if the player wins, -1 if the player loses, 0 for a draw or ongoing game.
        """
        if self.history_boring_turn_hash.count(self._hash()) >= 3:
            return 0
        if self.boring_turn >= 50:
            return 0
        if self.turn <= 10 and len(self.actions()) == 0:
            return -1 if player == self.current_player else 1
        if self.turn > 10 and not self._has_king(-self.current_player):
            return 1 if player == self.current_player else -1
        player_has_piece = self._has_piece(player)
        opponent_has_piece = self._has_piece(-player)
        if not player_has_piece and not opponent_has_piece:
            return 0
        if not player_has_piece:
            return -1
        if not opponent_has_piece:
            return 1
        return 0

    def __str__(self):
        s = '-' * (self.dim[1] * 5 + 1) + '\n'
        for i in range(0, self.dim[0]):
            local_s = '|'
            for j in range(0, self.dim[1]):
                if (i, j) in self.pieces:
                    local_s += f' {self.pieces[(i, j)]:2} |'
                else:
                    local_s += '    |'
            s += local_s + '\n'
            s += '-' * (self.dim[1] * 5 + 1) + '\n'
        return s

    def _flatten(self):
        board = [0 for _ in range(self.dim[0] * self.dim[1])]
        for position, value in self.pieces.items():
            board[position[0] * self.dim[1] + position[1]] = value
        return tuple(board)

    def _hash(self):
        if self.precomputed_hash is None:
            self.precomputed_hash = hash(self._flatten())
        return self.precomputed_hash

    class _ActionContainer:
        def __init__(self):
            self.actions = []
            self.max_captured_units = 0

        def add(self, action, captured_units):
            if captured_units > self.max_captured_units:
                self.max_captured_units = captured_units
                self.actions = [action]
            elif captured_units == self.max_captured_units:
                self.actions.append(action)

        def get_actions(self):
            return self.actions
