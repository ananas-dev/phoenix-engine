import ctypes

class Agent:
    def __init__(self, player):
        self.player = player
    
    def act(self, state, remaining_time):
        raise NotImplementedError
