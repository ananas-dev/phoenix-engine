import numpy as np
from scipy.optimize import minimize
from ia_agent import IAAgent

w = [0] * 360

agent = IAAgent(0, "../cmake-build-release/libclippy.so")

agent.set_weights(w)
agent.load_position_db("db2.txt")

# Dummy E function (replace with your real implementation)
def E(params):
    agent.set_weights([int(x) for x in params])
    return agent.evaluation_error()

# Local search (same as in Texel's pseudocode)
def local_optimize(initial_guess):
    best_params = list(initial_guess)
    best_E = E(best_params)
    improved = True

    while improved:
        improved = False
        for i in range(len(best_params)):
            for delta in [+1, -1]:
                test_params = best_params[:]
                test_params[i] += delta
                test_E = E(test_params)
                if test_E < best_E:
                    best_E = test_E
                    best_params = test_params
                    improved = True
                    break  # Try again from the beginning

    return best_params

# Hybrid optimization: Scipy (float) → Round → Local Search (int)
def hybrid_optimize(initial_guess):
    initial_guess = np.array(initial_guess, dtype=float)

    # Phase 1: Scipy optimization (float domain)
    bounds = [(-100, 100)] * 360
    bounds[336] = (0, 1000)
    bounds[347] = (0, 1000)
    bounds[348] = (0, 1000)
    result = minimize(E, initial_guess, method="Powell", bounds=bounds)
    float_solution = result.x

    print("Float solution: ", float_solution)
    np.savetxt("o6.txt", float_solution)

    # Round to integers
    rounded_solution = np.round(float_solution).astype(int).tolist()

    # Phase 2: Local search (int domain)
    final_solution = local_optimize(rounded_solution)
    return final_solution

# Example usage
initial = [0] * 360  # 10 evaluation parameters
final_params = hybrid_optimize(initial)
print("Final optimized parameters:", final_params)