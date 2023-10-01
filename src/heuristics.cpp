#include "heuristics.h"
#include <iostream>
/**
 * inserts the operation as early as possible into the machine -> active solution
 * @param operation
 * @param min_time -> job dependency
 * @return the starting time
 */
unsigned int Machine::insert(const std::shared_ptr<MNode>& operation, unsigned int min_time) {
    if (firstOp == nullptr) {
        firstOp = operation;
        operation->start = min_time;
        return operation->start;
    }

    auto current_op = firstOp;
    int time_gap_start = 0;
    while (current_op->next) {
        if (current_op->start - time_gap_start >= operation->duration && current_op->start - operation->duration >= min_time) {
            if (current_op->previous) {
                current_op->previous->next = operation;
                operation->previous = current_op->previous;
                operation->start = std::max(operation->previous->end(), min_time);
            } else {
                firstOp = operation;
                operation->start = min_time;
            }
            operation->next = current_op;
            current_op->previous = operation;
            return operation->start;
        }
        time_gap_start = current_op->end();
        current_op = current_op->next;
    }
    current_op->next = operation;
    operation->previous = current_op;
    operation->start = std::max(current_op->end(), min_time);
    return operation->start;
}

/**
 * get the operations job sequence
 * @return job sequence
 */
vector<int> Machine::getSequence() {
    auto sequence = vector<int>();
    auto current_op = firstOp;
    while (current_op->next) {
        sequence.emplace_back(current_op->job);
        current_op = current_op->next;
    }
    sequence.emplace_back(current_op->job);
    return sequence;
}


vector<vector<int>> Heuristics::random(JSSPInstance const &instance) {
    auto machines = vector<Machine>(instance.machineCount);
    auto rng = std::mt19937(instance.getSeed());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, instance.jobCount - 1);

    auto job_index = vector<int>();
    auto job_min_time = vector<long long>();
    for (int i = 0; i < instance.machineCount; i++) {
        job_index.emplace_back(0);
        job_min_time.emplace_back(0);
    }

    while (std::accumulate(job_index.begin(), job_index.end(), 0) < instance.operationCount()) {
        auto job_selected = dist(rng);
        while (job_index[job_selected] >= instance.machineCount) {
            job_selected = dist(rng);
        }
        auto jssp_instance_op = instance.instance[job_selected][job_index[job_selected]];
        auto operation = MNode(job_selected, jssp_instance_op.duration);
        auto start_time = machines[jssp_instance_op.machine].insert(std::make_shared<MNode>(operation), job_min_time[job_selected]);
        job_min_time[job_selected] = start_time + jssp_instance_op.duration;
        job_index[job_selected] += 1;
    }

    auto solution = vector<vector<int>>();
    for (auto machine: machines) {
        solution.emplace_back(machine.getSequence());
    }
    return solution;
}
