from ia_agent import IAAgent
import numpy as np


w = [0] * 305

agent = IAAgent(0, "../cmake-build-release/libclippy.so")

agent.set_weights(w)

checkpoint_idx = 0

def checkpoint(weights):
    weights_array = np.array(weights, dtype=int)
    np.savetxt("checkpoint_.txt", weights_array)

def E(w):
    agent.set_weights(w)
    return agent.evaluation_error("db.txt")

def local_optimize(initial_guess):
    n_params = len(initial_guess)
    best_params = initial_guess.copy()
    best_e = E(best_params)

    improved = True
    i = 0
    while improved:
        improved = False
        for pi in range(n_params):
            # Try incrementing the parameter
            new_params = best_params.copy()
            new_params[pi] += 1
            new_e = E(new_params)

            if new_e < best_e:
                best_e = new_e
                best_params = new_params.copy()
                improved = True
            else:
                # Try decrementing the parameter
                new_params = best_params.copy()
                new_params[pi] -= 1
                new_e = E(new_params)
                print(new_e)

                if new_e < best_e:
                    best_e = new_e
                    best_params = new_params.copy()
                    improved = True
        i += 1
        if i % 10 == 0:
            checkpoint(weights=best_params)

    return best_params

best = local_optimize(w)

print(best)