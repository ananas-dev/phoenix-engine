from agent import Agent
import fenix
import random as rd
import math
    

class MCTSAgent(Agent):
    def __init__(self, player):
        super().__init__(player)
        self.Q = {} # Total rewards of each node
        self.N = {} # Number of visits to each node
        self.children = {} # Children of each node
        self.actions_map = {} # Maps (state, child_state) to the action that led to the child
        self.exploration_constant = 2**0.5 # Exploration constant for UCT


    def act(self, state, remaining_time):
        if state.is_terminal():
            return None

        # Initialize the root state
        if state not in self.Q:
            self.Q[state] = 0
            self.N[state] = 0
            self.children[state] = set()
            self.actions_map[state] = {}

        # Build the action map for the current state
        for action in state.actions():
            child_state = state.result(action)
            self.actions_map[state][child_state] = action

        # Run MCTS simulations
        for _ in range(100):
            self.do_rollout(state)

        # Choose the best action directly
        best_action = self.choose_action(state)
        return best_action

    
    def choose_action(self, state):
        if state.is_terminal():
            return None
        
        # If state has no children yet, choose a random action
        if state not in self.children or not self.children[state]:
            return rd.choice(state.actions())
        
        # Find best child state
        def score(child):
            if self.N[child] == 0:
                return float('-inf')  # Changed from inf to -inf to prefer explored states
            return self.Q[child] / self.N[child]
        
        best_child = max(self.children[state], key=score)
        
        # Return the action that leads to the best child
        return self.actions_map[state][best_child]
        

    def do_rollout(self, state):
        path = self._select(state)
        leaf = path[-1]
        self._expand(leaf)
        reward = self._simulate(leaf)
        self._backpropagate(path, reward)

    def _select(self, state):
        path = []
        current_state = state
        
        while True:
            path.append(current_state)
            if current_state not in self.children or not self.children[current_state]:
                return path
            
            # Check for unexplored children
            unexplored = set()
            for child in self.children[current_state]:
                if child not in self.N or self.N[child] == 0:
                    unexplored.add(child)
            
            if unexplored:
                next_state = unexplored.pop()
                path.append(next_state)
                return path
            
            current_state = self._uct_select(current_state)
    

    def _expand(self, state):
        if state in self.children:
            return
        
        self.children[state] = set()
        self.actions_map[state] = {}
        
        for action in state.actions():
            child = state.result(action)
            self.children[state].add(child)
            self.actions_map[state][child] = action
            
            if child not in self.N:
                self.N[child] = 0
                self.Q[child] = 0

    def _simulate(self, state):
        current_state = state
        invert_reward = 1
        
        while not current_state.is_terminal():
            action = rd.choice(current_state.actions())
            current_state = current_state.result(action)
            invert_reward *= -1

        return invert_reward * current_state.utility(self.player)
    

    def _backpropagate(self, path, reward):
        for state in reversed(path):
            self.N[state] = self.N.get(state, 0) + 1
            self.Q[state] = self.Q.get(state, 0) + reward
            reward *= -1

    def _uct_select(self, state):
        log_N_vertex = math.log(self.N[state])

        def uct(child):
            if self.N[child] == 0:
                return float('inf')
            return (self.Q[child] / self.N[child]) + self.exploration_constant * math.sqrt(log_N_vertex / self.N[child])
        
        best_child = max(self.children[state], key=uct)
        return best_child