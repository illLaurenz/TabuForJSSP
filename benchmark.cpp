#include <iostream>
#include <fstream>
#include <chrono>
#include "src/jssp.h"
#include "src/ts.h"
#include "src/mem.h"
#include "src/heuristics.h"

using namespace std::chrono;

void memetic_bench(string instance_path, int time_limit, int lb, int seed) {
    auto t0 = high_resolution_clock::now();

    JSSPInstance instance = JSSPInstance(instance_path, seed);
    MemeticAlgorithm mem = MemeticAlgorithm(instance);
    BMResult result = mem.optimize(time_limit, lb);

    long long total_time = duration_cast<seconds>(high_resolution_clock::now() - t0).count();

    double time_to_msp = 0;
    for (auto e: result.history) {
        if (std::get<1>(e) == result.makespan) {
            time_to_msp = std::get<0>(e);
        }
    }
    std::ofstream out_file;
    out_file.open ("memetic_bm.txt", std::ios::app);
    out_file << instance_path << "\t" << "total time\t" << total_time << "s\t" << "makespan reached at\t" << time_to_msp << "s\t" << "makespan:\t" << result.makespan << "\n";
    out_file.close();
}

void tabu_bench(string instance_path, int time_limit, int lb, int seed) {
    auto t0 = high_resolution_clock::now();

    JSSPInstance instance = JSSPInstance(instance_path, seed);
    TabuSearch ts = TabuSearch(instance);
    auto heuristic_solution = Heuristics::random(instance);
    auto starting_solution = Solution{heuristic_solution, instance.calcMakespan(heuristic_solution)};
    BMResult result = ts.optimize(starting_solution, time_limit, lb);

    long long total_time = duration_cast<seconds>(high_resolution_clock::now() - t0).count();

    double time_to_msp = 0;
    for (auto e: result.history) {
        if (std::get<1>(e) == result.makespan) {
            time_to_msp = std::get<0>(e);
        }
    }

    std::ofstream out_file;
    out_file.open ("tabu_bm.txt", std::ios::app);
    out_file << instance_path << "\t" << "total time\t" << total_time << "s\t" << "makespan reached at\t" << time_to_msp << "s\t" << "makespan:\t" << result.makespan << "\n";
    out_file.close();
}

int main() {
    vector<string> instances = {"../instances/abz_instances/abz5.txt", "../instances/abz_instances/abz6.txt", "../instances/abz_instances/abz7.txt", "../instances/abz_instances/abz8.txt"
    ,"../instances/abz_instances/abz9.txt", "../instances/ft_instances/ft06.txt", "../instances/ft_instances/ft10.txt", "../instances/ft_instances/ft20.txt", "../instances/swv_instances/swv01.txt", "../instances/swv_instances/swv02.txt", "../instances/swv_instances/swv03.txt", "../instances/swv_instances/swv04.txt", "../instances/swv_instances/swv05.txt", "../instances/swv_instances/swv06.txt", "../instances/swv_instances/swv07.txt", "../instances/swv_instances/swv08.txt", "../instances/swv_instances/swv09.txt", "../instances/swv_instances/swv10.txt", "../instances/swv_instances/swv11.txt", "../instances/swv_instances/swv12.txt", "../instances/swv_instances/swv13.txt", "../instances/swv_instances/swv14.txt", "../instances/swv_instances/swv15.txt", "../instances/swv_instances/swv16.txt", "../instances/swv_instances/swv17.txt", "../instances/swv_instances/swv18.txt", "../instances/swv_instances/swv19.txt", "../instances/swv_instances/swv20.txt"};
    int time_limit = 300; // (seconds)
    int seed = 1234;

    for (auto &instance_path: instances) {
        tabu_bench(instance_path, time_limit, 0, seed);
        memetic_bench(instance_path, time_limit, 0, seed);
    }
}