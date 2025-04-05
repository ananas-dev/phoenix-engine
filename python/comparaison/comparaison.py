import fenix
import time
from copy import deepcopy
import numpy as np

import random_agent
import ia_agent
import mcts_agent
import minimax_agent

class ComparisonGameManager:
    def __init__(self, agent_1, agent_2, time_limit=300):
        self.agent_1 = agent_1
        self.remaining_time_1 = time_limit
        self.playout_times_1 = np.array([])

        self.agent_2 = agent_2
        self.remaining_time_2 = time_limit
        self.playout_times_2 = np.array([])

    def play(self):
        state = fenix.FenixState()

        turn = 0
        while not state.is_terminal() and self.remaining_time_1 >= 0 and self.remaining_time_2 >= 0:

            current_player = state.current_player
            agent, remaining_time = (self.agent_1, self.remaining_time_1) if state.current_player == 1 else (self.agent_2, self.remaining_time_2)

            action = None
            copy_state = deepcopy(state)
            start_time = time.perf_counter()
            action = agent.act(copy_state, remaining_time)
            elapsed_time = time.perf_counter() - start_time
            remaining_time -= elapsed_time
            if current_player == 1:
                self.playout_times_1 = np.append(self.playout_times_1, elapsed_time)
            else:
                self.playout_times_2 = np.append(self.playout_times_2, elapsed_time)

            valid_actions = state.actions()
            if action not in valid_actions:
                return -1 if state.to_move() == 1 else 1, -1 if state.to_move() == -1 else 1

            state = state.result(action)

            if current_player == 1:
                self.remaining_time_1 = remaining_time
            else:
                self.remaining_time_2 = remaining_time

            turn += 1

        if state.is_terminal():
            return state.utility(1), state.utility(-1)
        elif self.remaining_time_1 < 0:
            return -1, 1
        elif self.remaining_time_2 < 0:
            return 1, -1


if __name__ == "__main__":
    win_loss_matrix = np.zeros((4, 4))

    player_1 = [
        random_agent.RandomAgent(0),
        ia_agent.IAAgent(0),
        mcts_agent.MCTSAgent(0),
        minimax_agent.MinimaxAgent(0),
    ]

    player_2 = [
        random_agent.RandomAgent(1),
        ia_agent.IAAgent(1),
        mcts_agent.MCTSAgent(1),
        minimax_agent.MinimaxAgent(1),
    ]

    names = ["random_agent", "ia_agent", "mcts_agent", "minimax_agent"]

    csvs = [open(f"comparaison_data/{name}.csv", "a") for name in names]

    for csv in csvs:
        csv.write("turn_n, agent_time\n")

    for k in range(20):
        print(f"Game {k+1} / 50")
        print("===================================")
        for i, agent1 in enumerate(player_1):
            for j, agent2 in enumerate(player_2):
                if i == j:
                    continue
                manager = ComparisonGameManager(agent1, agent2)
                result = manager.play()
                if result[0] == 1:
                    win_loss_matrix[i][j] += 1
                    win_loss_matrix[j][i] -= 1
                elif result[0] == -1:
                    win_loss_matrix[i][j] -= 1
                    win_loss_matrix[j][i] += 1

                for turn, playout_time in enumerate(manager.playout_times_1):
                    csvs[i].write(f"{turn}, {playout_time}\n")

                for turn, playout_time in enumerate(manager.playout_times_2):
                    csvs[j].write(f"{turn}, {playout_time}\n")


    for csv in csvs:
        csv.close()

    win_loss_matrix_csv = open("comparaison_data/win_loss_matrix.csv", "w")
    for i in range(len(win_loss_matrix)):
        for j in range(len(win_loss_matrix[i])):
            win_loss_matrix_csv.write(f"{win_loss_matrix[i][j]},")
        win_loss_matrix_csv.write("\n")
    win_loss_matrix_csv.close()
    print(win_loss_matrix)