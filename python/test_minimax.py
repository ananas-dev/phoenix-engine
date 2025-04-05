import minimax_agent
import visual_game_manager
import random_agent

manager = visual_game_manager.VisualGameManager(
    red_agent=minimax_agent.MinimaxAgent(0, 3),
    black_agent=random_agent.RandomAgent(1),
    min_agent_play_time=0.5,
)

manager.play()