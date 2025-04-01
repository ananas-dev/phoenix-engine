import fenix
import time
from copy import deepcopy

class TextGameManager:
    def __init__(self, agent_1, agent_2, time_limit=300, display=True):
        self.agent_1 = agent_1
        self.remaining_time_1 = time_limit

        self.agent_2 = agent_2
        self.remaining_time_2 = time_limit

        self.dim = (7, 9)
        self.display = display

    def play(self):
        state = fenix.FenixState()

        if self.display:
            print(f"========== Initial State ==========")
            print(state)

        turn = 0
        while not state.is_terminal() and self.remaining_time_1 >= 0 and self.remaining_time_2 >= 0:

            current_player = state.current_player
            agent, remaining_time = (self.agent_1, self.remaining_time_1) if state.current_player == 1 else (self.agent_2, self.remaining_time_2)

            action = None
            copy_state = deepcopy(state)
            start_time = time.perf_counter()
            action = agent.act(copy_state, remaining_time)
            remaining_time -= time.perf_counter() - start_time

            valid_actions = state.actions()
            if action not in valid_actions:
                if self.display:
                    print(f"Invalid action: {action}")
                    print()
                    print(f"========== Game Over ==========")
                    print(f"Player 1 score: {-1 if state.to_move() == 1 else 1}")
                    print(f"Player -1 score: {-1 if state.to_move() == -1 else 1}")
                return -1 if state.to_move() == 1 else 1, -1 if state.to_move() == -1 else 1

            state = state.result(action)
            if self.display:
                print(f"========== Turn: {turn+1:3} ==========")
                print(f"\nChosen action: {action}\n")
                print(state)

            if current_player == 1:
                self.remaining_time_1 = remaining_time
            else:
                self.remaining_time_2 = remaining_time

            turn += 1

        if self.display:
            print(f"========== Game Over ==========")

        if state.is_terminal():
            if self.display:
                print(f"Player 1 score: {state.utility(1)}")
                print(f"Player -1 score: {state.utility(-1)}")
            return state.utility(1), state.utility(-1)
        elif self.remaining_time_1 < 0:
            if self.display:
                print(f"Player 1 ran out of time.")
            return -1, 1
        elif self.remaining_time_2 < 0:
            if self.display:
                print(f"Player -1 ran out of time.")
            return 1, -1
