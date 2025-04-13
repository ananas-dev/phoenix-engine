import numpy as np
from scipy.optimize import minimize_scalar
from ia_agent import IAAgent

w = [0] * 361
w[337] = 500
w[349] = 500
w[336] = 100
w[348] = 100
w[340] = 8
w[352] = 8
w[339] = 5
w[351] = 5
w[356] = 10


agent = IAAgent(0, "../cmake-build-release/libclippy.so")

# agent.set_weights(w)
agent.load_position_db("db2.txt")

# Dummy E function (replace with your real implementation)
def E(k):
    agent.set_weights(w)
    ret = agent.evaluation_error(k)
    print(ret)
    return ret

# Local search (same as in Texel's pseudocode)
result = minimize_scalar(E, bounds=(0, 1000))
float_solution = result.x
print(float_solution)