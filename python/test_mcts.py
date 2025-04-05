import mcts_agent
import visual_game_manager
import random_agent
import ia_agent

agent1 = mcts_agent.MCTSAgent(0)
agent2 = ia_agent.IAAgent(1)

manager = visual_game_manager.VisualGameManager(
    red_agent=mcts_agent.MCTSAgent(0),
    black_agent=ia_agent.IAAgent(1),
    min_agent_play_time=0.5,
)

manager.play()