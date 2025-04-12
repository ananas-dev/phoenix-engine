from ia_agent import IAAgent
import numpy as np
import multiprocessing as mp
import os

def checkpoint(weights):
    weights_array = np.array(weights, dtype=int)
    np.savetxt("checkpoint_.txt", weights_array)

def evaluate_single_change(param_index, direction, current_weights):
    """Evaluate a single parameter change in an isolated process"""
    # Create a new agent instance for this process
    agent = IAAgent(0, "../cmake-build-release/libclippy.so")
    
    # Make a copy of the weights and modify the specified parameter
    new_params = current_weights.copy()
    new_params[param_index] += direction
    
    # Evaluate with the modified weights
    agent.set_weights(new_params)
    error = agent.evaluation_error("db.txt")
    
    return (error, param_index, direction, new_params)

def parallel_optimize(initial_guess):
    # Use all available cores minus 1
    n_processes = max(1, mp.cpu_count() - 1)
    print(f"Using {n_processes} processes for optimization (Process ID: {os.getpid()})")
    
    n_params = len(initial_guess)
    best_params = initial_guess.copy()
    
    # Initialize an agent in the main process for initial evaluation
    agent = IAAgent(0, "../cmake-build-release/libclippy.so")
    agent.set_weights(best_params)
    best_e = agent.evaluation_error("db.txt")
    print(f"Initial error: {best_e}")
    
    improved = True
    iteration = 0
    
    while improved:
        improved = False
        pool = mp.Pool(processes=n_processes)
        tasks = []
        
        # Create all tasks to evaluate parameter changes
        for pi in range(n_params):
            # Task for incrementing the parameter
            tasks.append((pi, 1, best_params))
            # Task for decrementing the parameter
            tasks.append((pi, -1, best_params))
        
        # Execute all tasks in parallel
        results = pool.starmap(evaluate_single_change, tasks)
        
        # Always close the pool to free resources
        pool.close()
        pool.join()
        
        # Find the best result
        for error, param_idx, direction, new_params in results:
            if error < best_e:
                best_e = error
                best_params = new_params.copy()
                improved = True
                print(f"Improved: param {param_idx} {'+' if direction > 0 else '-'}, error: {error}")
        
        iteration += 1
        if iteration % 10 == 0 or improved:
            checkpoint(weights=best_params)
            print(f"Iteration {iteration}, best error: {best_e}")
    
    return best_params

if __name__ == "__main__":
    w = [0] * 305
    best = parallel_optimize(w)
    print("Final best weights:")
    print(best)
