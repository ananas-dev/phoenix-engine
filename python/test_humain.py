import visual_game_manager
import ia_agent
import random_agent

manager = visual_game_manager.VisualGameManager(
    # red_agent=random_agent.RandomAgent(0),
    black_agent=ia_agent.IAAgent(1),
    min_agent_play_time=0,
)

manager.play()
