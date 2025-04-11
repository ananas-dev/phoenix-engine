import ia_agent
from fen_utils import fen_to_state

state = fen_to_state("GSSSSS2/3GS2s/SKSS2ss/S1S2sgs/SS2ss1s/S2s1gss/3gss1g 8 w 00")
agent = ia_agent.IAAgent(0, "../cmake-build-release/libclippy.so", debug=True)

move = agent.act(state, 100)

print(move)
