#ifndef HYBRID_EVO_ALGORITHM_MEM_H
#define HYBRID_EVO_ALGORITHM_MEM_H

#include "jssp.h"
#include "ts.h"
#include <tuple>
#include <chrono>


class MemeticAlgorithm {
public:
    // instance -> JSSPInstance. populationSize = 30, tsIterations=12000 are recommended following the original paper
    MemeticAlgorithm(JSSPInstance &instance, int populationSize = 30, int tsIterations=12000) : instance(instance),
            tsIterations(tsIterations), populationSize(populationSize) {};

    // mainly method for testing functionality
    Solution optimize_iteration_constraint(int maxIterations);

    // optimize with a time limit in seconds, known optimum only for early stop (0 if unknown)
    BMResult optimize(int time_limit, int known_optimum);
    BMResult optimize_pop(int time_limit, vector<Solution> &start_solutions, int known_optimum=0);

private:
    JSSPInstance &instance;
    // iterations of each tabu search call. 12000 following the original paper
    const int tsIterations;
    // population size. 30 following the original paper
    const int populationSize;

    // logging intermediate makespans while running
    vector<std::tuple<double, int>> makespan_history;

    std::mt19937 rng;
    std::chrono::time_point<std::chrono::system_clock> t_start;
    vector<Solution> population;
    Solution currentBest;

    // init the population random at the start of the memetic algo
    void initializeRandPopulation();

    // create child solution from two parent solutions
    std::tuple<Solution, Solution> recombinationOperator(Solution const &p1, Solution const &p2);

    // find Longest Common Sequence of the two machines
    vector<int> crossover(vector<int> const &p1, vector<int> const &p2, vector<int> const &lcs);
    vector<int> dp_lcs(const vector<int> &m1, const vector<int> &m2);

    // calculate Similarity Degree of a solution to the current population
    int calcSimilarityDegree(int solutionIndex);

    // solution's value in the population
    float calcQualityScore(int makespann, int similarityDegree, int max_mksp, int min_mksp, int max_sim, int min_sim);
    float A(int max, int min, int value);

    // call at the end of each operation. takes a population with size = populationSize + 2
    // and removes the 2 individuals with the lowest quality score
    void updatePopulation();

    // log makespan method to save the time and new best makespan while running
    void logMakespan(int makespann);

    void optimizeLoop(int time_limit, int known_optimum, TabuSearch ts_algo);
};


#endif //HYBRID_EVO_ALGORITHM_MEM_H
