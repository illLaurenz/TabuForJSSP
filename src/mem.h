#ifndef HYBRID_EVO_ALGORITHM_MEM_H
#define HYBRID_EVO_ALGORITHM_MEM_H

#include "jssp.h"
#include "ts.h"
#include "heuristics.h"
#include <tuple>
#include <chrono>


class MemeticAlgorithm {
public:
    /**
     * initialize.
     * @param instance a valid JSSPInstance
     * @param population_size number of solutions which are observed simultaneously
     * @param tabu_search_iterations number of tabu search iterations executed on each new solution
     * @param quality_score_beta weight for solution quality rating in [0,1]. Higher -> more importance to makespan
     *                                                                        Lower -> more importance to similarity
     */
    explicit MemeticAlgorithm(JSSPInstance &instance, int population_size=30, int tabu_search_iterations=12000, float quality_score_beta=0.6) :
            instance(instance), populationSize(population_size), tabuSearchIterations(tabu_search_iterations), qualityScoreBeta(quality_score_beta),
            ts_algo(TabuSearch(instance)) {};

    // mainly method for testing functionality
    Solution optimizeIterationConstraint(int max_iterations);

    // optimize with a time limit in seconds, known optimum or LB for early stop (0 if unknown)
    BMResult optimize(int time_limit, int lower_bound=0);

    // optimize with a time limit in seconds, known optimum or LB for early stop and a population of staring solutions
    BMResult optimizePopulation(int time_limit, vector<Solution> &start_solutions, int lower_bound=0);

    // OPTIONAL: set tabu list parameters -> influence how long items are forbidden. See tabuList for details.
    void setTabuListParams(int _tt=2, int _d1=5, int _d2=12, unsigned int _tabuListSize= 0) {
        ts_algo.setTabuListParams(_tt, _d1, _d2, _tabuListSize);};

private:
    JSSPInstance &instance;
    TabuSearch ts_algo;
    // iterations of each tabu search call.
    const int tabuSearchIterations;
    // population size.
    const int populationSize;
    // constant for quality score weighting
    const float qualityScoreBeta;

    // logging intermediate makespans while running
    vector<std::tuple<double, int>> makespanHistory;

    // rng module
    std::mt19937 rng;
    // starting time for logging
    std::chrono::time_point<std::chrono::system_clock> tStart;
    // population for the memetic algorithm
    vector<Solution> population;
    // current best solution
    Solution currentBest;

    // main algorithm loop, called by both optimize and optimizePopulation
    void optimizeLoop(int time_limit, int known_optimum);

    // init the population random at the start of the memetic algo
    void initializeRandPopulation();

    // create child solution from two parent solutions
    std::tuple<Solution, Solution> recombinationOperator(Solution const &parent_1, Solution const &parent_2);

    // find Longest Common Sequence of the two machines
    static vector<int> crossover(vector<int> const &machine_parent_1, vector<int> const &machine_parent_2, vector<int> const &lcs);
    static vector<int> findLongestCommonSequence(const vector<int> &machine_1, const vector<int> &machine_2);

    // calculate Similarity Degree of a solution to the current population
    int calcSimilarityDegree(int solution_index);

    // call at the end of each operation. takes a population with size = populationSize + 2
    // and removes the 2 individuals with the lowest quality score
    void updatePopulation();

    // log makespan method to save the time and new best makespan while running
    void logMakespan(int makespan);

    // solution's value in the population
    [[nodiscard]] inline float calcQualityScore(int makespan, int similarity_degree, int max_makespan, int min_makespan, int max_similarity, int min_similarity) const {
        return qualityScoreBeta * A(max_makespan, min_makespan, makespan) + (1 - qualityScoreBeta) * A(max_similarity, min_similarity, similarity_degree);
    };

    // normalization function for score calculation
    inline static float A(int max, int min, int value) {
        return static_cast<float>(max - value) / static_cast<float>(max - min + 1);
    };

    // sorting function for population scoring
    static inline bool byQuality(std::tuple<float,int> t1, std::tuple<float,int> t2) {
        return std::get<0>(t1) < std::get<0>(t2);
    };

    // sorting function
    static inline bool byMakespanDecs(Solution const &a, Solution const &b) {
        return a.makespan < b.makespan;
    }
};


#endif //HYBRID_EVO_ALGORITHM_MEM_H
