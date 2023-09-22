#include <iostream>
#include "jssp.h"
#include "ts.h"
#include "mem.h"
#include <chrono>

using namespace std::chrono;
using std::cout, std::endl;

void memTest() {
    string instance_name = "../instances/abz_instances/abz5.txt";
    string solution_name = "../test_files/spt_abz5.txt";

    //JSSPInstance instance = JSSPInstance(instance_name,1234);
    //auto mem = MemeticAlgorithm(instance, 5);
    Solution solution; // = instance.readSolution(solution_name);

    int best = INT32_MAX;
    long avg_mksp = 0;
    auto t0 = high_resolution_clock::now();
    for (int i = 0; i < 1; i++) {
        JSSPInstance instance = JSSPInstance(instance_name);
        auto mem = MemeticAlgorithm(instance, 30);
        auto result = mem.optimize(30,1234);
        best = std::min(best, result.makespan);
        avg_mksp += result.makespan;
    }
    long long sumOld = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
    std::cout << "Time " << sumOld << "ms, avg: " << avg_mksp / 10 << ", best " << best << std::endl;

}

void tabuTest() {
    string instance_name = "../instances/abz_instances/abz5.txt";

    JSSPInstance instance = JSSPInstance(instance_name);
    auto ts = TabuSearch(instance);
    Solution solution; // = instance.readSolution(solution_name);

    int best = INT32_MAX;
    long avg_mksp = 0;
    auto t0 = high_resolution_clock::now();
    for (int i = 0; i < 10; i++) {
        solution = instance.generateRandomSolution();
        auto result = ts.optimize_it(solution, 12000);
        best = std::min(best, result.makespan);
        avg_mksp += result.makespan;
    }
    long long sumOld = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
    std::cout << "Time " << sumOld << "ms, avg: " << avg_mksp / 10 << ", best " << best << std::endl;
}


int main() {
    auto t_start = std::chrono::system_clock::now();
    for (int i = 0; i < 3; i++)
        memTest();

    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    cout << elapsed_seconds.count();
}