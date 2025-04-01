import fenix
import pygame
import sys
import random
import threading
import time
from copy import deepcopy

class VisualGameManager:
    """
    A visual game manager for the Fenix game.

    This class provides an interactive graphical interface for playing the game,
    supporting both human and AI players.

    If no action is selected, the user can click on a piece to see its possible moves. The user can the press the
    left and right arrow keys to cycle through the available actions.
    Pressing the Enter key will confirm the selected action.
    Pressing the Escape key will cancel the selected action or exit the game if no action is selected.
    Pressing the 'r' key will randomly select an action for the current player if it is a human player's turn.

    Attributes:
        red_agent (object): The AI agent for the red player (None if human-controlled).
        black_agent (object): The AI agent for the black player (None if human-controlled).
        total_time (int): Total time available for each player in seconds.
        min_agent_play_time (float): Minimum time an AI agent takes to play.

    Methods:
        handle_events(): Handles user inputs (mouse clicks, keyboard presses).
        update(): Updates the game state and processes agent actions.
        draw(): Renders the game board, pieces, and UI elements.
        play(): Runs the main game loop until the user quits. This method should be called to start the game.
    """

    def __init__(self, red_agent=None, black_agent=None, total_time=300 , min_agent_play_time=0.5):
        """
        Initializes the game manager and sets up the graphical interface.

        Args:
            red_agent (object, optional): AI agent for the red player (None for human control).
            black_agent (object, optional): AI agent for the black player (None for human control).
            total_time (int, optional): Total time per player in seconds (default: 300).
            min_agent_play_time (float, optional): Minimum agent thinking time (default: 0.5s).
        """
        self.dim = (7, 8)
        self.min_agent_play_time = min_agent_play_time

        self.red_agent = red_agent
        self.black_agent = black_agent

        self.state = fenix.FenixState()

        self.winner = None

        self.actions = self.state.actions()

        self.selected_actions = []
        self.selected_id = 0

        self.selected_action = None

        self.human_to_play = self.red_agent is None
        self.agent_thread = None
        self.agent_action = None
        self.time_start_thread = time.perf_counter_ns()

        self.remaining_time_red = total_time
        self.remaining_time_black = total_time

        pygame.init()
        self.screen = pygame.display.set_mode((70*self.dim[1] + 100, 70*self.dim[0] + 150))
        pygame.display.set_caption("Fenix")

        self.pieces_images = {
            3: pygame.image.load("pngs/king_red.png"),
            2: pygame.image.load("pngs/general_red.png"),
            1: pygame.image.load("pngs/soldier_red.png"),
            -1: pygame.image.load("pngs/soldier_black.png"),
            -2: pygame.image.load("pngs/general_black.png"),
            -3: pygame.image.load("pngs/king_black.png")
        }

        self.number_font = pygame.font.Font(None, 36)
        self.win_font = pygame.font.Font(None, 72)

        self.clock = pygame.time.Clock()
        self.running = True

        self.start_thinking_time = time.perf_counter_ns()

    def _handle_mouse_click(self, pos):
        row = (pos[1] - 50) // 70
        col = (pos[0] - 50) // 70
        if len(self.selected_actions) == 0 and self.human_to_play:
            start_action = {start for start, _, _ in self.actions}
            if (row, col) in start_action:
                self.selected_actions = [action for action in self.actions if action[0] == (row, col)]

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE and len(self.selected_actions) == 0:
                    self.running = False
                elif event.key == pygame.K_ESCAPE and len(self.selected_actions) > 0 and not self.state.is_terminal():
                    self.selected_actions = []
                    self.selected_id = 0
                elif event.key == pygame.K_RETURN and len(self.selected_actions) > 0 and not self.state.is_terminal():
                    self.selected_action = self.selected_actions[self.selected_id]
                elif event.key == pygame.K_LEFT and len(self.selected_actions) > 0 and not self.state.is_terminal():
                    self.selected_id = (self.selected_id - 1) % len(self.selected_actions)
                elif event.key == pygame.K_RIGHT and len(self.selected_actions) > 0 and not self.state.is_terminal():
                    self.selected_id = (self.selected_id + 1) % len(self.selected_actions)
                elif event.key == pygame.K_r and not self.state.is_terminal() and self.human_to_play:
                    self.selected_action = random.choice(self.actions)
            elif event.type == pygame.MOUSEBUTTONDOWN and not self.state.is_terminal():
                self._handle_mouse_click(event.pos)

    def _agent_thread(self):
        if self.human_to_play:
            raise ValueError("Human to play")
        agent = self.red_agent if self.state.current_player == 1 else self.black_agent
        remaining_time = self.remaining_time_red if self.state.current_player == 1 else self.remaining_time_black
        self.agent_action = agent.act(deepcopy(self.state), remaining_time)

    def update(self):
        if self.state.is_terminal() or self.remaining_time_red <= 0 or self.remaining_time_black <= 0:
            self.winner = self.state.utility(1)

            if self.winner == 0:
                if self.remaining_time_red <= 0:
                    self.winner = -1
                elif self.remaining_time_black <= 0:
                    self.winner = 1


            self.actions = []
            self.selected_actions = []
            self.selected_id = 0
            self.selected_action = None
        else:
            if not self.actions:
                self.actions = self.state.actions()
            if self.selected_action:

                if self.state.to_move() == 1:
                    self.remaining_time_red -= (time.perf_counter_ns() - self.start_thinking_time) * 1e-9
                else:
                    self.remaining_time_black -= (time.perf_counter_ns() - self.start_thinking_time) * 1e-9

                if self.selected_action not in self.actions:
                    raise ValueError("Invalid action")

                self.state = self.state.result(self.selected_action)
                self.actions = self.state.actions()
                self.selected_actions = []
                self.selected_id = 0
                self.selected_action = None

                self.human_to_play = (self.red_agent is None and self.state.to_move() == 1) or (self.black_agent is None and self.state.to_move() == -1)
                self.agent_thread = None

                self.start_thinking_time = time.perf_counter_ns()

            if not self.human_to_play and self.agent_thread is None and not self.state.is_terminal():
                self.agent_thread = threading.Thread(target=self._agent_thread)
                self.agent_thread.start()
                self.time_start_thread = time.perf_counter_ns()

            if not self.human_to_play and self.agent_thread is not None and not self.agent_thread.is_alive() and time.perf_counter_ns() - self.time_start_thread >= self.min_agent_play_time * 1e9:
                self.selected_action = self.agent_action

    def _draw_board(self):
        for i in range(self.dim[0]):
            for j in range(self.dim[1]):
                pygame.draw.rect(self.screen, 'Black', (70*j + 50, 70*i + 50, 70, 70), 1)

    def _draw_piece(self, position, value):
        stack_position = (70*position[1] + 55, 70*position[0] + 55)
        for i in range(abs(value)-1):
            image = self.pieces_images[1 if value > 0 else -1]
            self.screen.blit(image, stack_position)
            stack_position = (stack_position[0], stack_position[1] - 10)
        self.screen.blit(self.pieces_images[value], stack_position)

    def _draw_pieces(self):
        if len(self.selected_actions) > 0:
            start, end, removed = self.selected_actions[self.selected_id]
            for i in range(self.state.dim[0]):
                for j in range(self.state.dim[1]):
                    if (i, j) in self.state.pieces:
                        value = self.state.pieces[(i, j)]
                        self._draw_piece((i, j), value)

            pygame.draw.circle(self.screen, '#81a2c5', (70*start[1] + 85, 70*start[0] + 85), 8)
            pygame.draw.circle(self.screen, '#5783b2', (70*start[1] + 85, 70*start[0] + 85), 5)
            pygame.draw.circle(self.screen, '#77d373', (70*end[1] + 85, 70*end[0] + 85), 8)
            pygame.draw.circle(self.screen, '#49c445', (70*end[1] + 85, 70*end[0] + 85), 5)

            if removed:
                for pos in removed:
                    pygame.draw.line(self.screen, 'Red', (70*pos[1] + 55, 70*pos[0] + 55), (70*pos[1] + 115, 70*pos[0] + 115), 3)
                    pygame.draw.line(self.screen, 'Red', (70*pos[1] + 115, 70*pos[0] + 55), (70*pos[1] + 55, 70*pos[0] + 115), 3)

        else:
            start_action = {start for start, _, _ in self.actions}

            for i in range(self.state.dim[0]):
                for j in range(self.state.dim[1]):
                    if (i, j) in self.state.pieces:
                        value = self.state.pieces[(i, j)]
                        self._draw_piece((i, j), value)
                        if (i, j) in start_action and self.human_to_play:
                            pygame.draw.circle(self.screen, '#81a2c5', (70*j + 85, 70*i + 85), 8)
                            pygame.draw.circle(self.screen, '#5783b2', (70*j + 85, 70*i + 85), 5)

    def draw(self):
        self.screen.fill('White')

        self._draw_board()
        self._draw_pieces()

        if self.winner is not None:
            text = None
            if self.winner == 0:
                text = self.win_font.render("Draw!", True, 'Black')
            else:
                text = self.win_font.render(f"{'Red' if self.winner > 0 else 'Black'} wins!", True, 'Black')
            text_rect = text.get_rect(center=(self.screen.get_width()//2, 70*self.dim[0] + 100))
            pygame.draw.rect(self.screen, 'White', text_rect)
            self.screen.blit(text, text_rect)
        else:
            if self.human_to_play:
                text = self.number_font.render(f"{'Red' if self.state.to_move() == 1 else 'Black'} to play", True, 'Black')
                text_rect = text.get_rect(center=(self.screen.get_width()//2, 70*self.dim[0] + 75))
                self.screen.blit(text, text_rect)
            else:
                text = self.number_font.render(f"{'Red' if self.state.to_move() == 1 else 'Black'} AI is thinking...", True, 'Black')
                text_rect = text.get_rect(center=(self.screen.get_width()//2, 70*self.dim[0] + 75))
                self.screen.blit(text, text_rect)
            if len(self.selected_actions) > 0:
                text = self.number_font.render(f"Action {self.selected_id+1}/{len(self.selected_actions)}", True, 'Black')
                text_rect = text.get_rect(center=(self.screen.get_width()//2, 70*self.dim[0] + 100))
                self.screen.blit(text, text_rect)

            remaining_red = self.remaining_time_red
            remaining_red -= (time.perf_counter_ns() - self.start_thinking_time) * 1e-9 if self.state.to_move() == 1 else 0
            remaining_black = self.remaining_time_black
            remaining_black -= (time.perf_counter_ns() - self.start_thinking_time) * 1e-9 if self.state.to_move() == -1 else 0
            text = self.number_font.render(f"Red: {remaining_red:.2f} s", True, 'Black')
            text_rect = text.get_rect(center=(self.screen.get_width()//4, 70*self.dim[0] + 125))
            self.screen.blit(text, text_rect)
            text = self.number_font.render(f"Black: {remaining_black:.2f} s", True, 'Black')
            text_rect = text.get_rect(center=(3*self.screen.get_width()//4, 70*self.dim[0] + 125))
            self.screen.blit(text, text_rect)

        pygame.display.flip()

    def play(self):
        while self.running:
            self.handle_events()
            self.update()
            self.draw()
            self.clock.tick(60)

        pygame.quit()
        sys.exit()
