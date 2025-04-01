from agent import Agent
import random
import fenix

class RandomAgent(Agent):
    def act(self, state, remaining_time):
        actions = state.actions()
        if len(actions) == 0:
            raise Exception("No action available.")
        choice = random.choice(actions)
        print(choice)
        return choice
