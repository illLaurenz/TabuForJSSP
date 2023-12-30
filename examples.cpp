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
    auto heuristic_solution = Heuristics::random(instance);
    auto starting_solution = Solution{heuristic_solution, instance.calcMakespan(heuristic_solution)};
    BMResult result = ts.optimize(starting_solution, time_limit);

    JSSPInstance::writeSolutionToFile({result.solution, result.makespan}, "abz5-sol.txt");

    long long total_time = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
    std::cout << "Time " << total_time << ", Makespan " << result.makespan << std::endl;
}

int main() {
    string instance_path = "../instances/abz_instances/abz5.txt";
    int time_limit = 5; // (seconds)
    tabu_usage(instance_path, time_limit);
    memetic_usage(instance_path, time_limit);
}