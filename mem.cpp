#include <algorithm>
#include "mem.h"
#include <random>
#include <iostream>

void MemeticAlgorithm::logMakespan(int makespann) {
    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    makespan_history.emplace_back(std::tuple{elapsed_seconds.count(), makespann});
}

BMResult MemeticAlgorithm::optimize(int time_limit, int known_optimum) {
    t_start = std::chrono::system_clock::now();
    makespan_history = vector<std::tuple<double,int>>();
    population = vector<Solution>();

    currentBest = Solution{vector<vector<int>>(), INT32_MAX};
    initializeRandPopulation();
    TabuSearch ts_algo = TabuSearch(instance);
    for (auto &p: population) {
        auto elapsed_seconds = (std::chrono::system_clock::now() - t_start);
        if (elapsed_seconds.count() < time_limit && known_optimum != currentBest.makespan) {
            return BMResult{currentBest.solution, currentBest.makespan, makespan_history};
        }
        p = ts_algo.optimize_it(p, tsIterations);
        if (p.makespan < currentBest.makespan) {
            currentBest = p;
            logMakespan(currentBest.makespan);
        }
    }
    optimizeLoop(time_limit, known_optimum, ts_algo);
    return BMResult{currentBest.solution, currentBest.makespan, makespan_history};
}

BMResult MemeticAlgorithm::optimize_pop(int time_limit, vector<Solution> &start_solutions, int known_optimum) {
    t_start = std::chrono::system_clock::now();
    makespan_history = vector<std::tuple<double,int>>();
    population = std::move(start_solutions);

    currentBest = Solution{vector<vector<int>>(), INT32_MAX};
    initializeRandPopulation();
    TabuSearch ts_algo = TabuSearch(instance);
    for (auto &p: population) {
        p = ts_algo.optimize_it(p, tsIterations);
        if (p.makespan < currentBest.makespan) {
            currentBest = p;
            logMakespan(currentBest.makespan);
        }
    }
    optimizeLoop(time_limit, known_optimum, ts_algo);
    return BMResult{currentBest.solution, currentBest.makespan, makespan_history};

}

void MemeticAlgorithm::optimizeLoop(int time_limit, int known_optimum, TabuSearch ts_algo) {
    rng = std::mt19937(instance.getSeed());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,populationSize - 1);

    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    // main loop
    while (elapsed_seconds.count() < time_limit && known_optimum != currentBest.makespan) {
        auto p1 = dist(rng);
        auto p2 = dist(rng);
        while (p1 == p2) p2 = dist(rng);
        // critical
        auto children = recombinationOperator(population[p1], population[p2]);
        // end critical
        auto child1 = ts_algo.optimize_it(std::get<0>(children), tsIterations);
        auto child2 = ts_algo.optimize_it(std::get<1>(children), tsIterations);
        if (child1.makespan < currentBest.makespan) {
            currentBest = child1;
            logMakespan(currentBest.makespan);
        }
        if (child2.makespan < currentBest.makespan) {
            currentBest = child2;
            logMakespan(currentBest.makespan);
        }
        population.emplace_back(child1);
        population.emplace_back(child2);
        updatePopulation();

        elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    }
}

void MemeticAlgorithm::initializeRandPopulation() {
    while (population.size() < populationSize) {
        auto solution = instance.generateRandomSolution().solution;
        auto makespan = instance.calcMakespanAndFixSolution(solution);
        population.emplace_back(Solution{solution, makespan});
    }
}

std::tuple<Solution, Solution> MemeticAlgorithm::recombinationOperator(Solution const &p1, Solution const &p2) {
    auto child1_solution = vector<vector<int>>();
    auto child2_solution = vector<vector<int>>();
    for (int machine = 0; machine < instance.machineCount; machine++) {
        auto lcs = dp_lcs(p1.solution[machine],p2.solution[machine]);
        child1_solution.emplace_back(crossover(p1.solution[machine], p2.solution[machine], lcs));
        child2_solution.emplace_back(crossover(p2.solution[machine], p1.solution[machine], lcs));
    }
    auto makespan1 = instance.calcMakespanAndFixSolution(child1_solution, rng());
    auto makespan2 = instance.calcMakespanAndFixSolution(child2_solution, rng());
    return {Solution{child1_solution, makespan1}, Solution{child2_solution, makespan2}};
}

vector<int> MemeticAlgorithm::crossover(vector<int> const &p1, vector<int> const &p2, vector<int> const &lcs) {
    vector<int> p2_without_lcs = vector<int>();
    int i_lcs = 0;
    for (int job : p2) {
        if (job != lcs[i_lcs] || i_lcs >= lcs.size()) {
            p2_without_lcs.emplace_back(job);
        } else {
          ++i_lcs;
        }
    }
    i_lcs = 0;
    int i_p2 = 0;
    vector<int> child_sequence = vector<int>();
    for (int job : p1) {
        if (job == lcs[i_lcs] && i_lcs < lcs.size()) {
            child_sequence.emplace_back(job);
            ++i_lcs;
        } else  {
            child_sequence.emplace_back(p2_without_lcs[i_p2]);
            ++i_p2;
        }
    }
    return child_sequence;
}

vector<int> MemeticAlgorithm::dp_lcs(const vector<int> &m1, const vector<int> &m2) {
    int len = m1.size() + 1;
    auto dp = vector<vector<int>>();
    dp.emplace_back(vector<int>(len));
    for (int i = 1; i < len; i++) {
        dp.emplace_back(vector<int>(m1.size() + 1));
        for (int ii = 1; ii < len; ii++) {
            if (m1[i - 1] == m2[ii - 1]) {
                dp[i][ii] = dp[i - 1][ii - 1] + 1;
            } else {
                dp[i][ii] = std::max(dp[i - 1][ii], dp[i][ii - 1]);
            }
        }
    }
    auto lcs = vector<int>();
    for (int i = len - 1; i >= 0;) {
        for (int ii = len - 1; ii >= 0;) {
            if (dp[i][ii] == 0) {
                std::reverse(lcs.begin(), lcs.end());
                return lcs;
            } else if (dp[i][ii] > dp[i - 1][ii] && dp[i][ii] > dp[i][ii - 1]) {
                lcs.emplace_back(m1[i - 1]);
                --i;
                --ii;
            } else if (dp[i - 1][ii] > dp[i][ii - 1]) {
                --i;
            } else {
                --ii;
            }
        }
    }
    std::reverse(lcs.begin(), lcs.end());
    return lcs;
}

int MemeticAlgorithm::calcSimilarityDegree(int solutionIndex) {
    auto similarityDegree = vector<int>();
    for (int i = 0; i < population.size(); i++) {
        if (i == solutionIndex) continue;
        int sum = 0;
        for (int machine = 0; machine < population[i].solution.size(); machine++) {
            auto lcs = dp_lcs(population[solutionIndex].solution[machine], population[i].solution[machine]);
            sum += lcs.size();
        }
        similarityDegree.emplace_back(sum);
    }
    return *std::max_element(similarityDegree.begin(), similarityDegree.end());
}

float MemeticAlgorithm::calcQualityScore(int makespann, int similarityDegree, int max_mksp, int min_mksp, int max_sim, int min_sim) {
    float beta = 0.6;
    return beta * A(max_mksp, min_mksp, makespann) + (1 - beta) * A(max_sim, min_sim, similarityDegree);
}

float MemeticAlgorithm::A(int max, int min, int value) {
    return static_cast<float>(max - value) / static_cast<float>(max - min + 1);
}

bool byQuality(std::tuple<float,int> t1, std::tuple<float,int> t2) {
  return std::get<0>(t1) < std::get<0>(t2);
}

bool sort_by_makespann_decs(Solution a, Solution b) {
    return a.makespan < b.makespan;
}

void MemeticAlgorithm::updatePopulation() {
    vector<std::tuple<float,int>> qualityList = vector<std::tuple<float,int>>();
    vector<int> similarityDegrees = vector<int>();
    for (int i = 0; i < population.size(); i++) {
        similarityDegrees.emplace_back(calcSimilarityDegree(i));
    }
    int max_mksp = std::max_element(population.begin(), population.end(), sort_by_makespann_decs)->makespan;
    int min_mksp = std::min_element(population.begin(), population.end(), sort_by_makespann_decs)->makespan;
    int max_sim = *std::max_element(similarityDegrees.begin(), similarityDegrees.end());
    int min_sim = *std::min_element(similarityDegrees.begin(), similarityDegrees.end());
    for (int i = 0; i < population.size(); i++) {
        auto qualityScore = calcQualityScore(population[i].makespan, similarityDegrees[i], max_mksp, min_mksp, max_sim, min_sim);
        qualityList.emplace_back(std::tuple<float,int>{qualityScore,i});
    }
    std::sort(qualityList.begin(), qualityList.end(), byQuality);
    int e1 = std::get<0>(qualityList[0]);
    int e2 = std::get<0>(qualityList[1]);
    if (e1 < e2) {
        population.erase(population.begin() + e2);
        population.erase(population.begin() + e1);
    } else {
        population.erase(population.begin() + e1);
        population.erase(population.begin() + e2);
    }
}

Solution MemeticAlgorithm::optimize_iteration_constraint(int maxIterations) {
    int currentIteration = 0;
    population = vector<Solution>();
    currentBest = Solution{vector<vector<int>>(), INT32_MAX};
    initializeRandPopulation();
    TabuSearch ts_algo = TabuSearch(instance);
    for (auto &p: population) {
        p = ts_algo.optimize_it(p, tsIterations);
        if (p.makespan < currentBest.makespan) {
            currentBest = p;
        }
    }
    rng = std::mt19937(instance.getSeed());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,populationSize - 1);

    // main loop
    while (currentIteration++ < maxIterations) {
        auto p1 = dist(rng);
        auto p2 = dist(rng);
        while (p1 == p2) p2 = dist(rng);
        // critical
        auto children = recombinationOperator(population[p1], population[p2]);
        // end critical
        auto child1 = ts_algo.optimize_it(std::get<0>(children), tsIterations);
        auto child2 = ts_algo.optimize_it(std::get<1>(children), tsIterations);
        if (child1.makespan < currentBest.makespan) {
            currentBest = child1;
        }
        if (child2.makespan < currentBest.makespan) {
            currentBest = child2;
        }
        population.emplace_back(child1);
        population.emplace_back(child2);
        updatePopulation();
    }
    return currentBest;
}