from agent import Agent
import fenix

class MinimaxAgent(Agent):
    def __init__(self, player, depth):
        super().__init__(player)
        self.depth = depth

    def act(self, state, remaining_time):
        best_action = None
        best_value = float('-inf')
        for action in state.actions():
            next_state = state.result(action)
            value = self.min_value(next_state, self.depth)
            if value > best_value:
                best_value = value
                best_action = action
        return best_action

    def max_value(self, state, depth):
        if state.terminal_test():
            return state.utility(self.player)
        if depth == 0:
            return state.utility(self.player)
        value = float('-inf')
        for action in state.actions():
            value = max(value, self.min_value(state.result(action), depth - 1))
        return value

    def min_value(self, state, depth):
        if state.terminal_test():
            return state.utility(self.player)
        if depth == 0:
            return state.utility(self.player)
        value = float('inf')
        for action in state.actions():
            value = min(value, self.max_value(state.result(action), depth - 1))
        return value