#include "jssp.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>


vector<vector<Operation>> JSSPInstance::readInstance(string &filename) {
    vector<vector<Operation>> instance = vector<vector<Operation>>();

    std::ifstream file(filename);
    string line = "";
    std::getline(file, line);

    int split = line.find('\t');
    int jobs = std::stoi(line.substr(0, split));
    int machines = std::stoi(line.substr(split + 1));
    instance.reserve(jobs);
    for (int i = 0; i < jobs; i++) {
        instance.emplace_back(vector<Operation>());
        instance[i].reserve(machines);
    }

    int job = 0;
    while (std::getline(file, line)) {
        int pos = 0;
        do {
            int pos2 = line.find('\t', pos + 1);
            int machine = std::stoi(line.substr(pos, pos2));
            pos = line.find('\t', pos2 + 1);
            int duration = std::stoi(line.substr(pos2, pos++));
            instance[job].emplace_back(Operation{machine, duration, job});
        }while (pos);
        ++job;
    }
    file.close();
    return instance;
}

Solution JSSPInstance::readSolution(string &filename) {
    std::ifstream file(filename);
    string line;
    vector<vector<int>> solution = vector<vector<int>>();

    int machine = 0;
    while (std::getline(file, line)) {
        solution.emplace_back(vector<int>());
        int pos = 0;
        do {
            int pos2 = line.find('\t', pos + 1);
            int job = std::stoi(line.substr(pos, pos2));
            solution[machine].emplace_back(job);
            pos = pos2;
        } while(pos != -1);
        ++machine;
    }
    file.close();
    return {solution, calcMakespan(solution)};
}

int JSSPInstance::calcMakespan(vector<vector<int>> const &solution) const {
    vector<int> makespan_machine = vector<int>(solution.size());
    vector<int> sol_ptr = vector<int>(solution.size());
    vector<int> makespan_job = vector<int>(instance.size());
    vector<int> job_ptr = vector<int>(instance.size());

    int op_count = operationCount();
    while (op_count > 0) {
        for (int machine = 0; machine < solution.size(); machine++) {
            if (sol_ptr[machine] == solution[machine].size()) continue;
            int job = solution[machine][sol_ptr[machine]];
            if (instance[job][job_ptr[job]].machine == machine) {
                int makespan = std::max(makespan_job[job], makespan_machine[machine]) +
                               instance[job][job_ptr[job]].duration;
                makespan_job[job] = makespan;
                makespan_machine[machine] = makespan;
                ++sol_ptr[machine];
                ++job_ptr[job];
                --op_count;
            }
        }
    }
    return *std::max_element(makespan_machine.begin(), makespan_machine.end());
}

int JSSPInstance::calcMakespanAndFixSolution(vector<vector<int>>& solution, unsigned int _seed) const {
    vector<int> makespan_machine = vector<int>(solution.size());
    vector<int> sol_ptr = vector<int>(solution.size());
    vector<int> makespan_job = vector<int>(instance.size());
    vector<int> job_ptr = vector<int>(instance.size());

    std::mt19937 local_rnd;
    if (_seed == 0) {
        local_rnd = std::mt19937(seed);
    } else {
        local_rnd = std::mt19937(_seed);
    }

    int op_count = operationCount();
    int infeasibility_counter = 0;

    while (op_count > 0) {
        for (int machine = 0; machine < solution.size(); machine++) {
            if (sol_ptr[machine] == solution[machine].size()) continue;
            int job = solution[machine][sol_ptr[machine]];
            if (instance[job][job_ptr[job]].machine == machine) {
                int makespan = std::max(makespan_job[job], makespan_machine[machine]) +
                               instance[job][job_ptr[job]].duration;
                makespan_job[job] = makespan;
                makespan_machine[machine] = makespan;
                ++sol_ptr[machine];
                ++job_ptr[job];
                infeasibility_counter = 0;
                --op_count;
            } else {
                ++infeasibility_counter;
            }
        }
        if (infeasibility_counter > solution.size()) {
            recover_soulution(solution, sol_ptr, job_ptr, local_rnd);
        }
    }
    return *std::max_element(makespan_machine.begin(), makespan_machine.end());
}

int JSSPInstance::operationCount() const{
    int op_count = 0;
    for (auto &job: instance) {
        op_count += job.size();
    }
    return op_count;
}

void JSSPInstance::recover_soulution(vector<vector<int>> &solution, vector<int> &sol_ptr, vector<int> &job_ptr, std::mt19937 &local_rnd) const {
    vector<int> open_jobs = vector<int>();
    for (int i = 0; i < job_ptr.size(); i++) {
        if (job_ptr[i] < instance[i].size()) {
            open_jobs.emplace_back(i);
        }
    }
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, open_jobs.size() - 1);
    int job_no = open_jobs[dist(local_rnd)];
    Operation op = instance[job_no][job_ptr[job_no]];
    auto new_position = solution[op.machine].begin() + sol_ptr[op.machine];
    auto old_position = std::find(solution[op.machine].begin(), solution[op.machine].end(), op.job);
    solution[op.machine].erase(old_position);
    solution[op.machine].emplace(new_position, op.job);
}

std::tuple<int, int> JSSPInstance::readMetrics(string &filename) {
    std::ifstream file(filename);
    string line = "";
    std::getline(file, line);

    int split = line.find('\t');
    int jobs = std::stoi(line.substr(0, split));
    int machines = std::stoi(line.substr(split + 1));

    file.close();

    return {jobs, machines};
}

vector<int> JSSPInstance::randJobList(int size) {
    vector<int> joblist = vector<int>();
    for (int i = 0; i < size; i++) joblist.emplace_back(i);

    std::shuffle(joblist.begin(), joblist.end(), rng);

    return joblist;
}

Solution JSSPInstance::generateRandomSolution() {

    auto solution = vector<vector<int>>();
    for (int m = 0; m < machineCount; m++) {
        solution.emplace_back(randJobList(jobCount));
    }
    auto makespann = calcMakespanAndFixSolution(solution);

    return {solution, makespann};
}
