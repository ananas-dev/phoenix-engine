import mcts_agent
import game_manager
import random_agent

agent1 = mcts_agent.MCTSAgent(0)
agent2 = random_agent.RandomAgent(1)

manager = game_manager.TextGameManager(
    agent_1=agent1,
    agent_2=agent2,
)

for x in range(5000):
    print(f"-- GAME {x} --")
    manager.play()