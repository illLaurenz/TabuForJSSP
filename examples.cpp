#include <iostream>
#include <chrono>
#include "src/jssp.h"
#include "src/ts.h"
#include "src/mem.h"
#include "src/heuristics.h"

using namespace std::chrono;

void memetic_usage(string instance_path, int time_limit) {
    auto t0 = high_resolution_clock::now();

    JSSPInstance instance = JSSPInstance(instance_path);
    MemeticAlgorithm mem = MemeticAlgorithm(instance);
    BMResult result = mem.optimize(time_limit);

    long long total_time = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
    std::cout << "Time " << total_time << ", Makespan " << result.makespan << std::endl;
}

void tabu_usage(string instance_path, int time_limit) {
    auto t0 = high_resolution_clock::now();

    JSSPInstance instance = JSSPInstance(instance_path);
    TabuSearch ts = TabuSearch(instance);
    Solution starting_solution = instance.generateRandomSolution();
    BMResult result = ts.optimize(starting_solution, time_limit);

    long long total_time = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
    std::cout << "Time " << total_time << ", Makespan " << result.makespan << std::endl;
}

int main() {
    string instance_path = "../instances/abz_instances/abz5.txt";
    int time_limit = 5; // (seconds)
    /*long long sum = 0;
    for (auto i = 0; i < 100; i++) {
        auto instance = JSSPInstance(instance_path);
        auto solution = Heuristics::random(instance);
        auto msp = instance.calcMakespan(solution);
        sum += msp;
    }
    std::cout << sum / 100 << std::endl;
    sum = 0;
    for (auto i = 0; i < 100; i++) {
        auto instance = JSSPInstance(instance_path);
        auto solution = instance.generateRandomSolution();
        auto msp = solution.makespan;
        sum += msp;
    }
    std::cout << sum / 100 << std::endl;*/
    tabu_usage(instance_path, time_limit);
    //memetic_usage(instance_path, time_limit);
}