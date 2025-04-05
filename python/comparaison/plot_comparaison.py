import numpy as np
import matplotlib.pyplot as plt
import pandas as pd



TEST_DATA = False
data_dir = "comparaison/comparaison_data"

agent_names = ["random_agent", "ia_agent", "mcts_agent", "minimax_agent"]

names_to_label = {
    "random_agent": "Random Agent",
    "ia_agent": "IA Agent",
    "mcts_agent": "MCTS Agent",
    "minimax_agent": "Minimax Agent"
}

def load_mean_times(agent_name):
    data = pd.read_csv(f"{data_dir}/{agent_name}.csv", skipinitialspace=True)
    return data["turn_n"], data["agent_time"]

def load_win_loss_matrix():
    data = pd.read_csv(f"{data_dir}/win_loss_matrix.csv", header=None)
    win_loss_matrix = data.values
    return win_loss_matrix

def compute_mean_time(turns, times):
    mean_times = {} # Dictionary containing for each turn the mean time of the agent
    for turn in turns:
        if turn not in mean_times:
            mean_times[turn] = []
        mean_times[turn].append(times[turn])
    mean_times = {turn: np.mean(mean_times[turn]) for turn in mean_times}
    return mean_times

def plot_mean_times(mean_times):
    plt.figure(figsize=(10, 6))
    for agent_name, times in mean_times.items():
        # Convert the dictionary to a list of tuples and sort by turn number
        times = dict(sorted(times.items()))
        turns = list(times.keys())
        mean_times = list(times.values())
        plt.plot(turns, mean_times, label=names_to_label[agent_name], marker='o')
    plt.xlabel("Turn")
    plt.ylabel("Mean Time (s)")
    plt.title("Mean Time per Turn for Each Agent")
    plt.legend()
    plt.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.7)
    plt.savefig(f"{data_dir}/mean_times.svg")
    plt.show()


def plot_win_loss_matrix(win_loss_matrix):
    fig, ax = plt.subplots(figsize=(8, 6))
    cax = ax.matshow(win_loss_matrix, cmap='coolwarm', alpha=0.8)
    plt.colorbar(cax)
    ax.set_xticks(np.arange(len(agent_names)))
    ax.set_yticks(np.arange(len(agent_names)))
    ax.set_xticklabels(agent_names)
    ax.set_yticklabels(agent_names)
    plt.xlabel("Agent 1")
    plt.ylabel("Agent 2")
    plt.title("Win/Loss Matrix")
    plt.grid(False)
    plt.savefig(f"{data_dir}/win_loss_matrix.svg")
    plt.show()


if __name__ == "__main__":

    # if TEST_DATA:
    #     csvs = [open(f"{data_dir}/{name}.csv", "w") for name in agent_names]


    #     # generate test data for each agent
    #     for csv in csvs:
    #         csv.write("turn_n, agent_time\n") 
    #         for _ in range(10):
    #             turn = np.random.randint(0, 10)
    #             time = np.random.uniform(0, 5)
    #             csv.write(f"{turn}, {time}\n")

    #     win_loss_matrix = np.random.randint(-10, 11, (4, 4))
    #     win_loss_matrix_csv = open(f"{data_dir}/win_loss_matrix.csv", "w")
    #     for i in range(len(win_loss_matrix)):
    #         for j in range(len(win_loss_matrix[i])):
    #             win_loss_matrix_csv.write(f"{win_loss_matrix[i][j]},")
    #         win_loss_matrix_csv.write("\n")
    #     win_loss_matrix_csv.close()

    #     for csv in csvs:
    #         csv.close()

    mean_times = {}
    for agent_name in agent_names:
        turns, times = load_mean_times(agent_name)
        mean_times[agent_name] = compute_mean_time(turns, times)

    win_loss_matrix = load_win_loss_matrix()

    plot_mean_times(mean_times)
    plot_win_loss_matrix(win_loss_matrix)