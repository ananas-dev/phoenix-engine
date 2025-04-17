import clippy_agent
import fenix

agent = clippy_agent.ClippyAgent(0, debug=True)
agent.act(fenix.FenixState(), 10.0)
