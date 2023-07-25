#include "ts.h"
#include <algorithm>
#include <iostream>
#include <queue>

void TabuSearch::logMakespan(int makespan) {
    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    makespan_history.emplace_back(std::tuple{elapsed_seconds.count(), makespan});
}

// main method: resets algo vars and starts loop
Solution TabuSearch::optimize_it(Solution &solution, long max_iterations) {
    tabuList.reset();
    currentSolution = {solution.solution, solution.makespan};
    bestSolution = {solution.solution, solution.makespan};
    disjunctive_graph = generateDisjunctiveGraph();
    long iteration = 0;

    while (iteration++ <= max_iterations) {
        auto neighbourhood = generateNeighbourhood();
        tsMove(neighbourhood);
        if (currentSolution.makespan < bestSolution.makespan) {
            bestSolution = currentSolution;
        }
    }
    return bestSolution;
}

// main method: resets algo vars and starts loop
BMResult TabuSearch::optimize(Solution &solution, int seconds, int known_optimum) {
    t_start = std::chrono::system_clock::now();
    makespan_history = vector<std::tuple<double,int>>();

    tabuList.reset();
    currentSolution = {solution.solution, solution.makespan};
    bestSolution = {solution.solution, solution.makespan};
    logMakespan(bestSolution.makespan);

    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    // main loop
    while (elapsed_seconds.count() < seconds && known_optimum != bestSolution.makespan) {
        auto neighbourhood = generateNeighbourhood();
        tsMove(neighbourhood);
        if (currentSolution.makespan < bestSolution.makespan) {
            bestSolution = currentSolution;
            logMakespan(bestSolution.makespan);
        }
        elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    }
    return BMResult{bestSolution.solution, bestSolution.makespan, makespan_history};
}

// generates N7 like neighbourhood, with the difference that cyclic solutions are accepted and fixed
// more computational overhead than in the paper
vector<Neighbour> TabuSearch::generateNeighbourhood() {
    calcLongestPaths(disjunctive_graph);
    auto longest_path = findLongestPath(disjunctive_graph);
    auto block_list = generateBlockList(longest_path);
    auto neighbours_by_block = vector<vector<Neighbour>>();
    for (auto const &block: block_list) {
        neighbours_by_block.emplace_back(generateNeighboursFromBlock(block));
    }
    auto neighbourhood = vector<Neighbour>();
    for (auto const &b: neighbours_by_block) {
        for (auto const &n: b) {
            neighbourhood.emplace_back(n); // or move? or add in gen by block?
        }
    }
    return neighbourhood;
}

vector<vector<std::shared_ptr<Node>>> TabuSearch::generateDisjunctiveGraph() const {
    auto makespan_machine = vector<int>(instance.machineCount);
    auto sol_ptr = vector<int>(instance.machineCount);
    auto makespan_job = vector<int>(instance.jobCount);
    auto job_ptr = vector<int>(instance.jobCount);
    auto _disjunctive_graph = vector<vector<std::shared_ptr<Node>>>(instance.jobCount);
    auto disjunctive_arcs = vector<vector<std::shared_ptr<Node>>>(instance.machineCount);

    auto null_node = std::shared_ptr<Node>();

    int op_count = instance.operationCount();
    while (op_count > 0) {
        for (int machine = 0; machine < instance.machineCount; machine++) {
            if (sol_ptr[machine] == static_cast<int>(currentSolution.solution[machine].size())) continue;
            int job = currentSolution.solution[machine][sol_ptr[machine]];
            if (instance.instance[job][job_ptr[job]].machine == machine) {
                int start = std::max(makespan_job[job], makespan_machine[machine]);
                int duration = instance.instance[job][job_ptr[job]].duration;

                auto job_predecessor = _disjunctive_graph[job].empty() ? null_node : _disjunctive_graph[job].back();
                auto machine_predecessor = disjunctive_arcs[machine].empty() ? null_node : disjunctive_arcs[machine].back();
                auto node = std::make_shared<Node>(Node{job_predecessor, machine_predecessor, nullptr, nullptr, machine, job, start, duration, 0});
                if (job_predecessor) {
                    _disjunctive_graph[job].back()->job_successor = node;
                    _disjunctive_graph[job].emplace_back(node);
                } else {
                    _disjunctive_graph[job].emplace_back(node);
                }
                if (machine_predecessor) {
                    disjunctive_arcs[machine].back()->mach_successor = node;
                    disjunctive_arcs[machine].back() = node;
                }else {
                    disjunctive_arcs[machine].emplace_back(node);
                    disjunctive_arcs[machine].emplace_back(node);
                }

                makespan_job[job] = start + duration;
                makespan_machine[machine] = start + duration;
                ++sol_ptr[machine];
                ++job_ptr[job];
                --op_count;
            }
        }
    }
    return _disjunctive_graph;
}

void TabuSearch::calcLongestPaths(vector<vector<std::shared_ptr<Node>>> &d_graph) const {
    auto end_nodes = vector<std::shared_ptr<Node>>();
    for (auto &job: d_graph) {
        if (job.back()->mach_successor == nullptr) {
            end_nodes.emplace_back(job.back());
        }
    }
    std::sort(end_nodes.begin(), end_nodes.end(), sort_end_nodes);

    for (auto &e: end_nodes) {
        recursiveLPCalculation(e);
    }
}

void TabuSearch::recursiveLPCalculation(std::shared_ptr<Node> const &node) const {
    if (!node->mach_predecessor.expired()) {
        auto mp = node->mach_predecessor.lock();
        if (mp->len_to_n < node->len_to_n + node->duration) {
            mp->len_to_n = node->len_to_n + node->duration;
            recursiveLPCalculation(mp);
        }
    }
    if (!node->job_predecessor.expired()) {
        auto jp = node->job_predecessor.lock();
        if (jp->len_to_n < node->len_to_n + node->duration) {
            jp->len_to_n = node->len_to_n + node->duration;
            recursiveLPCalculation(jp);
        }
    }

}

vector<std::shared_ptr<Node>> TabuSearch::findLongestPath(const vector<vector<std::shared_ptr<Node>>>& d_graph) const {
    std::shared_ptr<Node> start_node;
    for (auto const &job: d_graph) {
        if (job.front()->len_to_n + job.front()->duration == currentSolution.makespan) {
            start_node = job.front();
        }
    }
    vector<std::shared_ptr<Node>> longest_path = {start_node};
    while (longest_path.back()->len_to_n != 0) {
        if (longest_path.back()->mach_successor) {
            auto len = longest_path.back()->mach_successor->len_to_n + longest_path.back()->mach_successor->duration;
            if (len == longest_path.back()->len_to_n) {
                longest_path.emplace_back(longest_path.back()->mach_successor);
                continue;
            }
        }
        if (longest_path.back()->job_successor) {
            auto len = longest_path.back()->job_successor->len_to_n + longest_path.back()->job_successor->duration;
            if (len == longest_path.back()->len_to_n) {
                longest_path.emplace_back(longest_path.back()->job_successor);
                continue;
            }
        }
    }
    return longest_path; // tidy return, is unreachable for feasible solutions
}

vector<vector<std::shared_ptr<Node>>> TabuSearch::generateBlockList(vector<std::shared_ptr<Node>> const &longest_path) {
    auto block_list = vector<vector<std::shared_ptr<Node>>>();
    auto block = vector<std::shared_ptr<Node>>();
    for (auto const &node: longest_path) {
        if (block.empty() || node->machine == block.back()->machine) {
            block.emplace_back(node);
        } else if (block.size() > 1){
            block_list.emplace_back(block);
            block.clear();
            block.emplace_back(node);
        } else {
            block.clear();
            block.emplace_back(node);
        }
    }
    if (block.size() > 1) block_list.emplace_back(block);
    return block_list;
}

vector<Neighbour> TabuSearch::generateNeighboursFromBlock(vector<std::shared_ptr<Node>> const &block) const {
    auto neighbours = vector<Neighbour>();
    int machine_no = block.front()->machine;
    auto const &machine_seq = currentSolution.solution[machine_no];
    auto start_index = std::find(machine_seq.begin(), machine_seq.end(), block.front()->job) - machine_seq.begin();

    if (block.size() == 2) {
        neighbours.emplace_back(forwardSwap(machine_seq, start_index, 0, 1, machine_no, block));
    } else {
        for (int u = 1; u < block.size() - 1; u++) {
            // move middle operations behind the last
            if (checkForwardSwap(block[u], block.back())) {
                neighbours.emplace_back(forwardSwap(machine_seq, start_index, u, block.size() - 1, machine_no, block));
            }
        }
        for (int v = 1; v < block.size(); v++) {
            // move first operation behind every operation
            if (checkForwardSwap(block.front(), block[v])) {
                neighbours.emplace_back(forwardSwap(machine_seq, start_index, 0, v, machine_no, block));
            }
        }
        for (int v = 1; v < block.size() - 1; v++) {
            // move middle operations before first
            if (checkBackwardSwap(block.front(), block[v])) {
                neighbours.emplace_back(backwardSwap(machine_seq, start_index, 0, v, machine_no, block));
            }
        }
        for (int u = 0; u < block.size() - 1; u++) {
            // move last operation before every operation
            if (checkBackwardSwap(block[u], block.back())) {
                neighbours.emplace_back(backwardSwap(machine_seq, start_index, u, block.size() - 1, machine_no, block));
            }
        }
    }
    return neighbours;
}

Neighbour TabuSearch::forwardSwap(vector<int> sequence, int const start_index, int const u, int const v, int const machine, vector<std::shared_ptr<Node>> const &block) {
    int item_u = sequence[start_index + u];
    sequence.erase(sequence.begin() + start_index + u);
    sequence.emplace(sequence.begin() + start_index + v, item_u);

    const int size = v - u + 1;
    auto len_to_i = vector<int>(size);
    auto len_from_i = vector<int>(size);

    {
        int jp_1 = 0;
        if (!block[u + 1]->job_predecessor.expired()) {
            auto jp = block[u + 1]->job_predecessor.lock();
            jp_1 = jp->start + jp->duration;
        }
        int mp_u = 0;
        if (!block[u]->mach_predecessor.expired()) {
            auto mp = block[u]->mach_predecessor.lock();
            mp_u = mp->start + mp->duration;
        }
        len_to_i[1] = std::max(jp_1, mp_u);
    }
    for (int w = 2; w < size; w++) {
        int jp_w = 0;
        if (!block[w]->job_predecessor.expired()) {
            auto jp = block[w]->job_predecessor.lock();
            jp_w = jp->start + jp->duration;
        }
        int y_mp = len_to_i[w - 1] + block[w - 1]->duration;
        len_to_i[w] = std::max(jp_w, y_mp);
    }
    {
        int jp_u = 0;
        if (!block[u]->job_predecessor.expired()) {
            auto jp = block[u]->job_predecessor.lock();
            jp_u = jp->start + jp->duration;
        }
        int y_v = len_to_i.back() + block[v]->duration;
        len_to_i[0] = std::max(jp_u, y_v);
    }

    {
        int js_u = !block[u]->job_successor ? 0 : block[u]->job_successor->len_to_n + block[u]->job_successor->duration;
        int ms_v = !block[v]->mach_successor ? 0 : block[v]->mach_successor->len_to_n + block[v]->mach_successor->duration;
        len_from_i[0] = std::max(js_u, ms_v) + block[u]->duration;
    }
    {
        int js_v = !block[v]->job_successor ? 0 : block[v]->job_successor->len_to_n + block[v]->job_successor->duration;
        int y_u = len_from_i[0];
        len_from_i.back() = std::max(js_v, y_u) + block[v]->duration;
    }
    for (int w = size - 2; w > 0; w--) {
        int js_w = !block[w]->job_successor ? 0 : block[w]->job_successor->len_to_n + block[w]->job_successor->duration;
        int ms_w = len_from_i[w + 1];
        len_from_i[w] = std::max(js_w, ms_w) + block[w]->duration;
    }

    int approx_makespan = 0;
    for (int i = 0; i < size; i++) {
        approx_makespan = std::max(approx_makespan, len_to_i[i] + len_from_i[i]);
    }
    if (v - u == 1) return {sequence, machine, approx_makespan, start_index + u, start_index+ v, adjacent};
    return {sequence, machine, approx_makespan, start_index + u, start_index+ v, forward};
}

Neighbour TabuSearch::backwardSwap(vector<int> sequence, int const start_index, int const u, int const v, int const machine, vector<std::shared_ptr<Node>> const &block) {
    int v_item = sequence[start_index + v];
    sequence.erase(sequence.begin() + start_index + v);
    sequence.emplace(sequence.begin() + start_index + u, v_item);

    const int size = v - u + 1;
    auto len_to_i = vector<int>(size);
    auto len_from_i = vector<int>(size);

    {
        int jp_v = 0;
        if (!block[v]->job_predecessor.expired()) {
            auto jp = block[v]->job_predecessor.lock();
            jp_v = jp->start + jp->duration;
        }
        int mp_u = 0;
        if (!block[u]->mach_predecessor.expired()) {
            auto mp = block[u]->mach_predecessor.lock();
            mp_u = mp->start + mp->duration;
        }
        len_to_i.back() = std::max(jp_v, mp_u);
    }
    {
        int jp_u = 0;
        if (!block[u]->job_predecessor.expired()) {
            auto jp = block[u]->job_predecessor.lock();
            jp_u = jp->start + jp->duration;
        }
        int y_v = len_to_i.back() + block[v]->duration;
        len_to_i[0] = std::max(jp_u, y_v);
    }
    for (int w = 1; w < size - 1; w++) {
        int jp_w = block[w]->start + block[w]->duration;
        int y_mp = len_to_i[w - 1] + block[w - 1]->duration;
        len_to_i[w] = std::max(jp_w, y_mp);
    }

    {
        int l = size - 2;
        int js_l = !block[l]->job_successor ? 0 : block[l]->job_successor->len_to_n + block[l]->job_successor->duration;
        int ms_v = !block[v]->mach_successor ? 0 : block[v]->mach_successor->len_to_n + block[v]->mach_successor->duration;
        len_from_i[l] = std::max(js_l, ms_v) + block[l]->duration;
    }
    for (int w = size - 3; w >= 0; w--) {
        int js_w = !block[w]->job_successor ? 0 : block[w]->job_successor->len_to_n + block[w]->job_successor->duration;
        int y_ms = len_from_i[w + 1];
        len_from_i[w] = std::max(js_w, y_ms) + block[w]->duration;
    }
    {
        int js_v = !block[v]->job_successor ? 0 : block[v]->job_successor->len_to_n + block[v]->job_successor->duration;
        int y_u = len_from_i[0];
        len_from_i.back() = std::max(js_v, y_u) + block[v]->duration;
    }

    int approx_makespan = 0;
    for (int i = 0; i < size; i++) {
        approx_makespan = std::max(approx_makespan, len_to_i[i] + len_from_i[i]);
    }
    if (v - u == 1) return {sequence, machine, approx_makespan, start_index + u, start_index + v, adjacent};
    return {sequence, machine, approx_makespan, start_index + u, start_index + v, backward};
}

// the tabu move. sort neighbourhood by approximated makespan, check for aspiration, or take best non tabu solution
// then calculate for exact makespan, if aspiration, check aspiration and tabu again
// if neither, choose random
bool TabuSearch::tsMove(vector<Neighbour> &neighbourhood) {
    if (neighbourhood.empty()) {
        return false;
    }
    std::sort(neighbourhood.begin(), neighbourhood.end(), compareNeighbours);
    for (auto &neighbour: neighbourhood) {
        if (neighbour.makespan < bestSolution.makespan) {
            // aspiration
            auto candidate_solution = currentSolution.solution;
            candidate_solution[neighbour.machine] = neighbour.sequence;
            int exact_makespan = instance.calcMakespan(candidate_solution);
            if (exact_makespan >= bestSolution.makespan && tabuList.isTabu(neighbour)) continue;

            updateCurrentSolution(neighbour);
            tabuList.updateTabuList(neighbour, bestSolution.makespan);
            currentSolution.solution[neighbour.machine] = neighbour.sequence;
            return true;
        } else if (!tabuList.isTabu(neighbour)) {
            // best non tabu
            updateCurrentSolution(neighbour);
            tabuList.updateTabuList(neighbour, bestSolution.makespan);
            currentSolution.solution[neighbour.machine] = neighbour.sequence;
            return false;
        }
    }
    // chose random, if all tabu
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,neighbourhood.size() - 1);
    auto randIndex = dist(rng);
    auto &neighbour = neighbourhood[randIndex];

    updateCurrentSolution(neighbour);
    tabuList.updateTabuList(neighbour, bestSolution.makespan);
    currentSolution.solution[neighbour.machine] = neighbour.sequence;
    return false;
}

void TabuSearch::updateCurrentSolution(Neighbour &neighbour) {
    // find first operation of the machine which is altered
    std::shared_ptr<Node> n1, n2;
    int job_n1 = currentSolution.solution[neighbour.machine][neighbour.start_pos];
    for (auto &node: disjunctive_graph[job_n1]) {
        if (node->machine == neighbour.machine) n1 = node;
    }

    int job_n2 = currentSolution.solution[neighbour.machine][neighbour.end_pos];
    for (auto &node: disjunctive_graph[job_n2]) {
        if (node->machine == neighbour.machine) n2 = node;
    }

    // execute swap move: rearrange pointers
    if (neighbour.swap == forward) {
        auto buff_mp = n1->mach_predecessor.expired() ? nullptr : n1->mach_predecessor.lock();
        auto buff_ms = n1->mach_successor;
        n1->mach_predecessor = n2;
        n1->mach_successor = n2->mach_successor;
        if (n1->mach_successor) n1->mach_successor->mach_predecessor = n1;
        n2->mach_successor = n1;

        if (buff_mp) buff_mp->mach_successor = buff_ms;
        if (buff_ms) buff_ms->mach_predecessor = buff_mp;
    } else if (neighbour.swap == backward){
        auto buff_mp = n2->mach_predecessor.expired() ? nullptr : n2->mach_predecessor.lock();
        auto buff_ms = n2->mach_successor;
        n2->mach_predecessor = n1->mach_predecessor;
        n2->mach_successor = n1;
        if (!n2->mach_predecessor.expired()) n2->mach_predecessor.lock()->mach_successor = n2;
        n1->mach_predecessor = n2;

        if (buff_mp) buff_mp->mach_successor = buff_ms;
        if (buff_ms) buff_ms->mach_predecessor = buff_mp;
    } else {
        auto buff_mp = n1->mach_predecessor.expired() ? nullptr : n1->mach_predecessor.lock();
        n1->mach_successor = n2->mach_successor;
        if (n1->mach_successor) n1->mach_successor->mach_predecessor = n1;
        n1->mach_predecessor = n2;

        n2->mach_predecessor = buff_mp;
        n2->mach_successor = n1;
        if (buff_mp) buff_mp->mach_successor = n2;
    }

    // leftshift / recalculate starting times
    // reset all longest paths to dummy end node
    vector<std::shared_ptr<Node>> startNodes = vector<std::shared_ptr<Node>>();
    startNodes.reserve(2 * instance.machineCount * instance.jobCount);
    for (auto &job: disjunctive_graph) {
        if (job.front()->mach_predecessor.expired()) startNodes.emplace_back(job.front());
        for (auto &node: job) {
            node->len_to_n = 0;
            node->start = 0;
        }
    }
    long long pos = 0;
    while (pos < startNodes.size()) {
        auto &node = startNodes[pos];
        auto &ms = node->mach_successor;
        auto &js = node->job_successor;
        auto end = node->start + node->duration;
        if (ms && ms->start < end) {
            ms->start = end;
            startNodes.emplace_back(ms);
        }
        if (js && js->start < end) {
            js->start = end;
            startNodes.emplace_back(js);
        }
        ++pos;
    }
    auto makespan = 0;
    for (auto &job: disjunctive_graph) {
        makespan = std::max(makespan, job.back()->start + job.back()->duration);
    }
    currentSolution.makespan = makespan;
}
