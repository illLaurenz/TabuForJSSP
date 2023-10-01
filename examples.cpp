#include <iostream>
#include "jssp.h"
#include "ts.h"
#include "mem.h"
#include <chrono>

using namespace std::chrono;
using std::cout, std::endl;

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
    int time_limit = 60; // (seconds)

    tabu_usage(instance_path, time_limit);
    memetic_usage(instance_path, time_limit);
}