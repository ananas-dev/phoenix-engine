from fen_utils import fen_to_state

state = fen_to_state("K1GS4/GG1SS3/2SS3S/1SS5/SS6/S2s3g/5gs1 49 b 11")

print(state)
print(state.actions())