#include <algorithm>
#include "mem.h"
#include <random>
#include <iostream>

void MemeticAlgorithm::logMakespan(int makespan) {
    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - tStart);
    makespanHistory.emplace_back(std::tuple{elapsed_seconds.count(), makespan});
}

BMResult MemeticAlgorithm::optimize(int time_limit, int known_optimum) {
    tStart = std::chrono::system_clock::now();
    makespanHistory = vector<std::tuple<double,int>>();
    population = vector<Solution>();

    currentBest = Solution{vector<vector<int>>(), INT32_MAX};
    initializeRandPopulation();
    TabuSearch ts_algo = TabuSearch(instance);
    for (auto &p: population) {
        auto elapsed_seconds = (std::chrono::system_clock::now() - tStart);
        if (elapsed_seconds.count() < time_limit && known_optimum != currentBest.makespan) {
            return BMResult{currentBest.solution, currentBest.makespan, makespanHistory};
        }
        p = ts_algo.optimize_it(p, tabuSearchIterations);
        if (p.makespan < currentBest.makespan) {
            currentBest = p;
            logMakespan(currentBest.makespan);
        }
    }
    optimizeLoop(time_limit, known_optimum, ts_algo);
    return BMResult{currentBest.solution, currentBest.makespan, makespanHistory};
}

BMResult MemeticAlgorithm::optimizePopulation(int time_limit, vector<Solution> &start_solutions, int known_optimum) {
    tStart = std::chrono::system_clock::now();
    makespanHistory = vector<std::tuple<double,int>>();
    population = std::move(start_solutions);

    currentBest = Solution{vector<vector<int>>(), INT32_MAX};
    initializeRandPopulation();
    TabuSearch ts_algo = TabuSearch(instance);
    for (auto &p: population) {
        p = ts_algo.optimize_it(p, tabuSearchIterations);
        if (p.makespan < currentBest.makespan) {
            currentBest = p;
            logMakespan(currentBest.makespan);
        }
    }
    optimizeLoop(time_limit, known_optimum, ts_algo);
    return BMResult{currentBest.solution, currentBest.makespan, makespanHistory};

}

void MemeticAlgorithm::optimizeLoop(int time_limit, int known_optimum, TabuSearch ts_algo) {
    rng = std::mt19937(instance.getSeed());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,populationSize - 1);

    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - tStart);
    // main loop
    while (elapsed_seconds.count() < time_limit && known_optimum != currentBest.makespan) {
        auto p1 = dist(rng);
        auto p2 = dist(rng);
        while (p1 == p2) p2 = dist(rng);

        auto children = recombinationOperator(population[p1], population[p2]);

        auto child1 = ts_algo.optimize_it(std::get<0>(children), tabuSearchIterations);
        auto child2 = ts_algo.optimize_it(std::get<1>(children), tabuSearchIterations);
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

        elapsed_seconds = (std::chrono::system_clock::now() - tStart);
    }
}

void MemeticAlgorithm::initializeRandPopulation() {
    while (population.size() < populationSize) {
        population.emplace_back(instance.generateRandomSolution());
    }
}

std::tuple<Solution, Solution> MemeticAlgorithm::recombinationOperator(Solution const &parent_1, Solution const &parent_2) {
    auto child_1_solution = vector<vector<int>>();
    auto child_2_solution = vector<vector<int>>();
    for (int machine = 0; machine < instance.machineCount; machine++) {
        auto lcs = findLongestCommonSequence(parent_1.solution[machine], parent_2.solution[machine]);
        child_1_solution.emplace_back(crossover(parent_1.solution[machine], parent_2.solution[machine], lcs));
        child_2_solution.emplace_back(crossover(parent_2.solution[machine], parent_1.solution[machine], lcs));
    }
    auto makespan_1 = instance.calcMakespanAndFixSolution(child_1_solution, rng());
    auto makespan_2 = instance.calcMakespanAndFixSolution(child_2_solution, rng());
    return {Solution{child_1_solution, makespan_1}, Solution{child_2_solution, makespan_2}};
}

vector<int> MemeticAlgorithm::crossover(vector<int> const &machine_parent_1, vector<int> const &machine_parent_2, vector<int> const &lcs) {
    vector<int> p2_without_lcs = vector<int>();
    int i_lcs = 0;
    for (int job : machine_parent_2) {
        if (job != lcs[i_lcs] || i_lcs >= lcs.size()) {
            p2_without_lcs.emplace_back(job);
        } else {
          ++i_lcs;
        }
    }
    i_lcs = 0;
    int i_p2 = 0;
    vector<int> child_sequence = vector<int>();
    for (int job : machine_parent_1) {
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

vector<int> MemeticAlgorithm::findLongestCommonSequence(const vector<int> &machine_1, const vector<int> &machine_2) {
    auto len = machine_1.size() + 1;
    auto dp_matrix = vector<vector<int>>();
    dp_matrix.emplace_back(vector<int>(len));
    for (auto i = 1; i < len; i++) {
        dp_matrix.emplace_back(vector<int>(machine_1.size() + 1));
        for (int ii = 1; ii < len; ii++) {
            if (machine_1[i - 1] == machine_2[ii - 1]) {
                dp_matrix[i][ii] = dp_matrix[i - 1][ii - 1] + 1;
            } else {
                dp_matrix[i][ii] = std::max(dp_matrix[i - 1][ii], dp_matrix[i][ii - 1]);
            }
        }
    }
    auto lcs = vector<int>();
    for (auto i = len - 1; i >= 0;) {
        for (auto ii = len - 1; ii >= 0;) {
            if (dp_matrix[i][ii] == 0) {
                std::reverse(lcs.begin(), lcs.end());
                return lcs;
            } else if (dp_matrix[i][ii] > dp_matrix[i - 1][ii] && dp_matrix[i][ii] > dp_matrix[i][ii - 1]) {
                lcs.emplace_back(machine_1[i - 1]);
                --i;
                --ii;
            } else if (dp_matrix[i - 1][ii] > dp_matrix[i][ii - 1]) {
                --i;
            } else {
                --ii;
            }
        }
    }
    std::reverse(lcs.begin(), lcs.end());
    return lcs;
}

int MemeticAlgorithm::calcSimilarityDegree(int solution_index) {
    auto similarity_degree = vector<int>();
    for (int i = 0; i < population.size(); i++) {
        if (i == solution_index) continue;
        int sum = 0;
        for (int machine = 0; machine < population[i].solution.size(); machine++) {
            auto lcs = findLongestCommonSequence(population[solution_index].solution[machine],
                                                 population[i].solution[machine]);
            sum += lcs.size();
        }
        similarity_degree.emplace_back(sum);
    }
    return *std::max_element(similarity_degree.begin(), similarity_degree.end());
}

void MemeticAlgorithm::updatePopulation() {
    vector<std::tuple<float,int>> quality_list = vector<std::tuple<float,int>>();
    vector<int> similarity_degrees = vector<int>();
    for (int i = 0; i < population.size(); i++) {
        similarity_degrees.emplace_back(calcSimilarityDegree(i));
    }
    int max_makespan = std::max_element(population.begin(), population.end(), byMakespanDecs)->makespan;
    int min_makespan = std::min_element(population.begin(), population.end(), byMakespanDecs)->makespan;
    int max_similarity = *std::max_element(similarity_degrees.begin(), similarity_degrees.end());
    int min_similarity = *std::min_element(similarity_degrees.begin(), similarity_degrees.end());
    for (int i = 0; i < population.size(); i++) {
        auto qualityScore = calcQualityScore(population[i].makespan, similarity_degrees[i], max_makespan, min_makespan, max_similarity, min_similarity);
        quality_list.emplace_back(std::tuple<float,int>{qualityScore, i});
    }
    std::sort(quality_list.begin(), quality_list.end(), byQuality);
    int e1 = std::get<0>(quality_list[0]);
    int e2 = std::get<0>(quality_list[1]);
    if (e1 < e2) {
        population.erase(population.begin() + e2);
        population.erase(population.begin() + e1);
    } else {
        population.erase(population.begin() + e1);
        population.erase(population.begin() + e2);
    }
}

Solution MemeticAlgorithm::optimizeIterationConstraint(int max_iterations) {
    int current_iteration = 0;
    population = vector<Solution>();
    currentBest = Solution{vector<vector<int>>(), INT32_MAX};
    initializeRandPopulation();
    TabuSearch ts_algo = TabuSearch(instance);
    for (auto &p: population) {
        p = ts_algo.optimize_it(p, tabuSearchIterations);
        if (p.makespan < currentBest.makespan) {
            currentBest = p;
        }
    }
    rng = std::mt19937(instance.getSeed());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,populationSize - 1);

    // main loop
    while (current_iteration++ < max_iterations) {
        auto parent_1 = dist(rng);
        auto parent_2 = dist(rng);
        while (parent_1 == parent_2) parent_2 = dist(rng);

        auto children = recombinationOperator(population[parent_1], population[parent_2]);

        auto child_1 = ts_algo.optimize_it(std::get<0>(children), tabuSearchIterations);
        auto child_2 = ts_algo.optimize_it(std::get<1>(children), tabuSearchIterations);
        if (child_1.makespan < currentBest.makespan) {
            currentBest = child_1;
        }
        if (child_2.makespan < currentBest.makespan) {
            currentBest = child_2;
        }
        population.emplace_back(child_1);
        population.emplace_back(child_2);
        updatePopulation();
    }
    return currentBest;
}