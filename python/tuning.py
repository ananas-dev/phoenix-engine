from deap import base, creator, tools
from game_manager import TextGameManager
from ia_agent import IAAgent
import random
import numpy as np

import multiprocessing

# ---- Engine interaction placeholder ----
def evaluate_matchup(args):
    i, j, ind1, ind2 = args

    w_1 = [round(x * 100) for x in ind1]
    w_2 = [round(x * 100) for x in ind2]

    agent_1 = IAAgent(0, "../cmake-build-release/libclippy.so")
    agent_2 = IAAgent(0, "../cmake-build-release/libclippy.so")

    agent_1.set_weights(w_1)
    agent_2.set_weights(w_2)

    manager = TextGameManager(agent_1, agent_2, display=False)
    result = manager.play()

    manager = TextGameManager(agent_2, agent_1, display=False)
    result_rev = manager.play()

    def compute_reward(score, turns):
        if score == 1:     # Win
            return 1 + (1.0 / turns)
        elif score == 0.0: # Draw
            return 0
        elif score == -1:
            return -1

    fitness_1 = compute_reward(result[0], result[2]) + compute_reward(result_rev[1], result_rev[2])
    fitness_2 = compute_reward(result[1], result[2]) + compute_reward(result_rev[0], result_rev[2])

    del agent_1
    del agent_2

    return i, j, fitness_1, fitness_2

# ---- GA Setup ----
NUM_WEIGHTS = 2
POP_SIZE = 20
NUM_GENERATIONS = 1
TOURNAMENT_SIZE = 3
MUTATION_RATE = 0.2
MUTATION_SIGMA = 0.1

creator.create("FitnessMax", base.Fitness, weights=(1.0,))
creator.create("Individual", list, fitness=creator.FitnessMax)

toolbox = base.Toolbox()
toolbox.register("attr_float", random.uniform, -1, 1)
toolbox.register("individual", tools.initRepeat, creator.Individual, toolbox.attr_float, n=NUM_WEIGHTS)
toolbox.register("population", tools.initRepeat, list, toolbox.individual)

def round_robin_fitness(population):
    n = len(population)
    jobs = [(i, j, population[i], population[j]) for i in range(n) for j in range(i + 1, n)]
    scores = toolbox.map(evaluate_matchup, jobs)

    fitnesses = [0.0] * len(population)
    counts = [0] * len(population)

    for i, j, fi, fj in scores:
        fitnesses[i] += fi
        fitnesses[j] += fj
        counts[i] += 1
        counts[j] += 1


    for i in range(len(population)):
        if counts[i] > 0:
            population[i].fitness.values = (fitnesses[i] / counts[i],)
        else:
            population[i].fitness.values = (0.0,)

toolbox.register("mate", tools.cxBlend, alpha=0.5)
toolbox.register("mutate", tools.mutGaussian, mu=0, sigma=MUTATION_SIGMA, indpb=MUTATION_RATE)
toolbox.register("select", tools.selTournament, tournsize=TOURNAMENT_SIZE)

pool = multiprocessing.Pool(processes=15)
toolbox.register("map", pool.map)

# ---- Main loop ----
def run():
    pop = toolbox.population(n=POP_SIZE)

    for gen in range(NUM_GENERATIONS):
        print(f"-- Generation {gen} --")

        round_robin_fitness(pop)
        fits = [ind.fitness.values[0] for ind in pop]
        print(f"  Best fitness: {max(fits):.3f}, Avg: {sum(fits)/len(fits):.3f}")

        offspring = toolbox.select(pop, len(pop))
        offspring = list(map(toolbox.clone, offspring))

        for child1, child2 in zip(offspring[::2], offspring[1::2]):
            if random.random() < 0.5:
                toolbox.mate(child1, child2)
                del child1.fitness.values
                del child2.fitness.values

        for mutant in offspring:
            if random.random() < 0.5:
                toolbox.mutate(mutant)
                del mutant.fitness.values

        pop[:] = offspring

        best = tools.selBest(pop, 1)[0]
        print("Best individual:", np.round(np.array(best) * 100))


    best = tools.selBest(pop, 1)[0]
    print("Best individual:", np.round(np.array(best) * 100))
    np.savetxt("best.txt", np.round(np.array(best) * 100))
    print("Fitness:", best.fitness.values[0])

if __name__ == "__main__":
    run()
