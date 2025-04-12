import multiprocessing
from copy import deepcopy
import fenix
from ia_agent import state_to_fen, IAAgent


def play_game(agent_1, agent_2):
    state = fenix.FenixState()

    agent_1.new_game()
    agent_2.new_game()

    positions = []

    while not state.is_terminal():
        agent = agent_1 if state.current_player == 1 else agent_2

        copy_state = deepcopy(state)
        if state.turn < 10:
            action = agent.act_random(copy_state, -1)
        else:
            action = agent.act(copy_state, -1)

        if action not in copy_state.actions():
            print("Engine played an invalid action")
            print("State turn:", state.turn)
            print("Action:", action)
            exit()

        state = state.result(action)

        if not agent.found_mate() and state.turn >= 10:
            positions.append(state_to_fen(state))

    if state.is_terminal():
        if state.utility(1) == 1:
            return 1, positions
        if state.utility(1) == 0:
            return 0.5, positions
        return 0, positions


def run_worker(worker_id, games_per_worker):
    agent_1 = IAAgent(0, "../cmake-build-release/libclippy.so")
    agent_2 = IAAgent(1, "../cmake-build-release/libclippy.so")

    with open(f"positions_worker_{worker_id}.txt", "w") as f:
        for i in range(games_per_worker):
            game_index = worker_id * games_per_worker + i

            if game_index % 2 == 0:
                res, positions = play_game(agent_1, agent_2)
            else:
                res, positions = play_game(agent_2, agent_1)

            for pos in positions:
                f.write(f"{pos},{res}\n")


def main():
    total_games = 64000
    num_workers = 15
    games_per_worker = total_games // num_workers

    processes = []
    for worker_id in range(num_workers):
        p = multiprocessing.Process(
            target=run_worker,
            args=(worker_id, games_per_worker)
        )
        p.start()
        processes.append(p)

    for p in processes:
        p.join()


if __name__ == "__main__":
    main()