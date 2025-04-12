from ia_agent import IAAgent


w = [0] * 305

agent = IAAgent(0, "../cmake-build-release/libclippy.so")

agent.set_weights(w)

def E(w):
    agent.set_weights(w)
    return agent.evaluation_error("sample.txt")

def local_optimize(initial_guess):
    n_params = len(initial_guess)
    best_params = initial_guess.copy()
    best_e = E(best_params)

    improved = True
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

    return best_params

best = local_optimize(w)

print(best)