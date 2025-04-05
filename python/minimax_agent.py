from agent import Agent
import fenix

class MinimaxAgent(Agent):
    def __init__(self, player, depth):
        super().__init__(player)
        self.depth = depth

    def act(self, state, remaining_time):
        best_action = None
        best_value = float('-inf')
        alpha = float('-inf')
        beta = float('inf')
        
        for action in state.actions():
            next_state = state.result(action)
            value = self.min_value(next_state, self.depth - 1, alpha, beta)
            
            if value > best_value:
                best_value = value
                best_action = action
            
            alpha = max(alpha, best_value)
        
        return best_action

    def max_value(self, state, depth, alpha, beta):
        if state.is_terminal():
            return state.utility(self.player)
        
        if depth == 0:
            return state.utility(self.player)
        
        value = float('-inf')
        
        for action in state.actions():
            next_state = state.result(action)
            value = max(value, self.min_value(next_state, depth - 1, alpha, beta))
            
            if value >= beta:
                return value  # Beta cutoff
            
            alpha = max(alpha, value)
        
        return value

    def min_value(self, state, depth, alpha, beta):
        if state.is_terminal():
            return state.utility(self.player)
        
        if depth == 0:
            return state.utility(self.player)
        
        value = float('inf')
        
        for action in state.actions():
            next_state = state.result(action)
            value = min(value, self.max_value(next_state, depth - 1, alpha, beta))
            
            if value <= alpha:
                return value  # Alpha cutoff
            
            beta = min(beta, value)
        
        return value