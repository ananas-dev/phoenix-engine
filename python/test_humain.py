import visual_game_manager
import random_agent

manager = visual_game_manager.VisualGameManager(
    black_agent=random_agent.RandomAgent(1),
)

manager.play()
