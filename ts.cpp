#include "ts.h"
#include <algorithm>
#include <queue>

/**
 * internal function for logging the makespan
 * @param makespan
 */
void TabuSearch::logMakespan(int makespan) {
    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - startTime);
    makespanHistory.emplace_back(std::tuple{elapsed_seconds.count(), makespan});
}

/**
 * iteration constrained tabu search
 * @param solution starting solution
 * @param max_iterations
 * @return solution struct: tabular solution, makespan
 */
Solution TabuSearch::optimize_it(Solution &solution, long max_iterations) {
    tabuList.reset();
    currentSolution = solution;
    bestSolution = solution;
    disjunctiveGraph = generateDisjunctiveGraph();
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

/**
 * time constrained tabu search
 * @param solution starting solution
 * @param seconds maximum runtime, soft limit
 * @param known_optimum best known solution / lower bound for early stop when found
 * @return BMResult struct: solution, makespan, history (solution - time log)
 */
BMResult TabuSearch::optimize(Solution &solution, int seconds, int known_optimum) {
    startTime = std::chrono::system_clock::now();
    makespanHistory = vector<std::tuple<double,int>>();

    tabuList.reset();
    currentSolution = solution;
    bestSolution = solution;
    logMakespan(bestSolution.makespan);

    auto elapsed_seconds = (std::chrono::system_clock::now() - startTime);
    // main loop
    while (elapsed_seconds.count() < seconds && known_optimum != bestSolution.makespan) {
        auto neighbourhood = generateNeighbourhood();
        tsMove(neighbourhood);
        if (currentSolution.makespan < bestSolution.makespan) {
            bestSolution = currentSolution;
            logMakespan(bestSolution.makespan);
        }
        elapsed_seconds = (std::chrono::system_clock::now() - startTime);
    }
    return BMResult{bestSolution.solution, bestSolution.makespan, makespanHistory};
}

/**
 * create a N7 neighbourhood with approximated makespans for current solution
 * @return neighbourhood
 */
vector<Neighbour> TabuSearch::generateNeighbourhood() {
    calcLongestPaths(disjunctiveGraph);
    auto longest_path = findLongestPath(disjunctiveGraph);
    auto block_list = generateBlockList(longest_path);

    auto neighbourhood = vector<Neighbour>();
    for (auto const &block: block_list) {
        for (auto const &neighbour: generateNeighboursFromBlock(block))
            neighbourhood.emplace_back(neighbour);
    }
    return neighbourhood;
}

/**
 * sets len_to_n for each node in the disjunctive graph.
 * they are used to find the longest path, create a feasible N7 neighbourhood and approximate the makespan of each
 * neighbour. See Zhang et al. for details, linked in README.md
 * @param d_graph the disjunctive graph
 */
void TabuSearch::calcLongestPaths(vector<vector<std::shared_ptr<Node>>> &d_graph) const {
    auto end_nodes = vector<std::shared_ptr<Node>>();
    for (auto &job: d_graph) {
        if (job.back()->machSuccessor == nullptr) {
            end_nodes.emplace_back(job.back());
        }
    }
    std::sort(end_nodes.begin(), end_nodes.end(), compNodesByEndTime);

    for (auto &node: end_nodes) {
        recursiveLongestPathCalculation(node);
    }
}

/**
 * recursive function for the len_to_n calculation. see TabuSearch::calcLongestPaths
 * recursive calculation is started at the node with the largest starting time, so each node is visited only once.
 * @param node the current node
 */
void TabuSearch::recursiveLongestPathCalculation(std::shared_ptr<Node> const &node) const {
    if (!node->machPredecessor.expired()) {
        auto mp = node->machPredecessor.lock();
        if (mp->lenToN < node->lenToN + node->duration) {
            mp->lenToN = node->lenToN + node->duration;
            recursiveLongestPathCalculation(mp);
        }
    }
    if (!node->jobPredecessor.expired()) {
        auto jp = node->jobPredecessor.lock();
        if (jp->lenToN < node->lenToN + node->duration) {
            jp->lenToN = node->lenToN + node->duration;
            recursiveLongestPathCalculation(jp);
        }
    }

}

/**
 * use len_to_n of the disjunctive graph to find a longest path in the instance
 * @param d_graph the disjunctive graph
 * @return a longest path of the disjunctive graph
 */
vector<std::shared_ptr<Node>> TabuSearch::findLongestPath(const vector<vector<std::shared_ptr<Node>>>& d_graph) const {
    std::shared_ptr<Node> start_node;
    for (auto const &job: d_graph) {
        if (job.front()->lenToN + job.front()->duration == currentSolution.makespan) {
            start_node = job.front();
        }
    }
    vector<std::shared_ptr<Node>> longest_path = {start_node};
    while (longest_path.back()->lenToN != 0) {
        if (longest_path.back()->machSuccessor) {
            auto len = longest_path.back()->machSuccessor->lenToN + longest_path.back()->machSuccessor->duration;
            if (len == longest_path.back()->lenToN) {
                longest_path.emplace_back(longest_path.back()->machSuccessor);
                continue;
            }
        }
        if (longest_path.back()->jobSuccessor) {
            auto len = longest_path.back()->jobSuccessor->lenToN + longest_path.back()->jobSuccessor->duration;
            if (len == longest_path.back()->lenToN) {
                longest_path.emplace_back(longest_path.back()->jobSuccessor);
                continue;
            }
        }
    }
    return longest_path;
}

/**
 * generate neighbourhood blocks of the longest path, to create a neighbourhood from it.
 * block: operations of the longest path, which are consecutive on the same machine
 * @param longest_path see TabuSearch::findLongestPath
 * @return vector of blocks
 */
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

/**
 * test if N7 swap results in a feasible solution, add machine sequence as neighbour and approximate makespan
 * @param block see TabuSearch::generateBlockList
 * @return a list of legal neighbours with approximated makespans
 */
vector<Neighbour> TabuSearch::generateNeighboursFromBlock(vector<std::shared_ptr<Node>> const &block) const {
    auto neighbours = vector<Neighbour>();
    int machine_no = block.front()->machine;
    auto const &machine_seq = currentSolution.solution[machine_no];
    int start_index = static_cast<int>(std::find(machine_seq.begin(), machine_seq.end(), block.front()->job) - machine_seq.begin());

    if (block.size() == 2) {
        neighbours.emplace_back(forwardSwap(machine_seq, start_index, 0, 1, machine_no, block));
    } else {
        for (int u = 1; u < block.size() - 1; u++) {
            // move middle operations behind the last
            if (checkForwardSwap(block[u], block.back())) {
                neighbours.emplace_back(forwardSwap(machine_seq, start_index, u, static_cast<int>(block.size() - 1), machine_no, block));
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
                neighbours.emplace_back(backwardSwap(machine_seq, start_index, u, static_cast<int>(block.size() - 1), machine_no, block));
            }
        }
    }
    return neighbours;
}

/**
 * approximate makespan for a forward swap on a block of the current solution
 * @param sequence
 * @param start_index
 * @param u
 * @param v
 * @param machine
 * @param block
 * @return
 */
Neighbour TabuSearch::forwardSwap(vector<int> sequence, int const start_index, int const u, int const v, int const machine, vector<std::shared_ptr<Node>> const &block) {
    int item_u = sequence[start_index + u];
    sequence.erase(sequence.begin() + start_index + u);
    sequence.emplace(sequence.begin() + start_index + v, item_u);

    const int size = v - u + 1;
    auto len_to_i = vector<int>(size);
    auto len_from_i = vector<int>(size);

    {
        int jp_1 = 0;
        if (!block[u + 1]->jobPredecessor.expired()) {
            auto jp = block[u + 1]->jobPredecessor.lock();
            jp_1 = jp->start + jp->duration;
        }
        int mp_u = 0;
        if (!block[u]->machPredecessor.expired()) {
            auto mp = block[u]->machPredecessor.lock();
            mp_u = mp->start + mp->duration;
        }
        len_to_i[1] = std::max(jp_1, mp_u);
    }
    for (int w = 2; w < size; w++) {
        int jp_w = 0;
        if (!block[w]->jobPredecessor.expired()) {
            auto jp = block[w]->jobPredecessor.lock();
            jp_w = jp->start + jp->duration;
        }
        int y_mp = len_to_i[w - 1] + block[w - 1]->duration;
        len_to_i[w] = std::max(jp_w, y_mp);
    }
    {
        int jp_u = 0;
        if (!block[u]->jobPredecessor.expired()) {
            auto jp = block[u]->jobPredecessor.lock();
            jp_u = jp->start + jp->duration;
        }
        int y_v = len_to_i.back() + block[v]->duration;
        len_to_i[0] = std::max(jp_u, y_v);
    }

    {
        int js_u = !block[u]->jobSuccessor ? 0 : block[u]->jobSuccessor->lenToN + block[u]->jobSuccessor->duration;
        int ms_v = !block[v]->machSuccessor ? 0 : block[v]->machSuccessor->lenToN + block[v]->machSuccessor->duration;
        len_from_i[0] = std::max(js_u, ms_v) + block[u]->duration;
    }
    {
        int js_v = !block[v]->jobSuccessor ? 0 : block[v]->jobSuccessor->lenToN + block[v]->jobSuccessor->duration;
        int y_u = len_from_i[0];
        len_from_i.back() = std::max(js_v, y_u) + block[v]->duration;
    }
    for (int w = size - 2; w > 0; w--) {
        int js_w = !block[w]->jobSuccessor ? 0 : block[w]->jobSuccessor->lenToN + block[w]->jobSuccessor->duration;
        int ms_w = len_from_i[w + 1];
        len_from_i[w] = std::max(js_w, ms_w) + block[w]->duration;
    }

    int approx_makespan = 0;
    for (int i = 0; i < size; i++) {
        approx_makespan = std::max(approx_makespan, len_to_i[i] + len_from_i[i]);
    }

    SwapDirection swap_direction = (v - u == 1) ? adjacent : forward;
    return {sequence, machine, approx_makespan, start_index + u, start_index+ v, swap_direction};
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
        if (!block[v]->jobPredecessor.expired()) {
            auto jp = block[v]->jobPredecessor.lock();
            jp_v = jp->start + jp->duration;
        }
        int mp_u = 0;
        if (!block[u]->machPredecessor.expired()) {
            auto mp = block[u]->machPredecessor.lock();
            mp_u = mp->start + mp->duration;
        }
        len_to_i.back() = std::max(jp_v, mp_u);
    }
    {
        int jp_u = 0;
        if (!block[u]->jobPredecessor.expired()) {
            auto jp = block[u]->jobPredecessor.lock();
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
        int js_l = !block[l]->jobSuccessor ? 0 : block[l]->jobSuccessor->lenToN + block[l]->jobSuccessor->duration;
        int ms_v = !block[v]->machSuccessor ? 0 : block[v]->machSuccessor->lenToN + block[v]->machSuccessor->duration;
        len_from_i[l] = std::max(js_l, ms_v) + block[l]->duration;
    }
    for (int w = size - 3; w >= 0; w--) {
        int js_w = !block[w]->jobSuccessor ? 0 : block[w]->jobSuccessor->lenToN + block[w]->jobSuccessor->duration;
        int y_ms = len_from_i[w + 1];
        len_from_i[w] = std::max(js_w, y_ms) + block[w]->duration;
    }
    {
        int js_v = !block[v]->jobSuccessor ? 0 : block[v]->jobSuccessor->lenToN + block[v]->jobSuccessor->duration;
        int y_u = len_from_i[0];
        len_from_i.back() = std::max(js_v, y_u) + block[v]->duration;
    }

    int approx_makespan = 0;
    for (int i = 0; i < size; i++) {
        approx_makespan = std::max(approx_makespan, len_to_i[i] + len_from_i[i]);
    }

    SwapDirection swap_direction = (v - u == 1) ? adjacent : backward;
    return {sequence, machine, approx_makespan, start_index + u, start_index + v, swap_direction};
}

// the tabu move. sort neighbourhood by approximated makespan, check for aspiration, or take best non tabu solution
// then calculate for exact makespan, if aspiration, check aspiration and tabu again
// if neither, choose random
bool TabuSearch::tsMove(vector<Neighbour> &neighbourhood) {
    if (neighbourhood.empty()) {
        return false;
    }
    std::sort(neighbourhood.begin(), neighbourhood.end(), compNeighboursByMakespan);
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
    auto rand_index = dist(rng);
    auto &neighbour = neighbourhood[rand_index];

    updateCurrentSolution(neighbour);
    tabuList.updateTabuList(neighbour, bestSolution.makespan);
    currentSolution.solution[neighbour.machine] = neighbour.sequence;
    return false;
}

void TabuSearch::updateCurrentSolution(Neighbour &neighbour) {
    // find first operation of the machine which is altered
    std::shared_ptr<Node> node1, node2;
    int job_node1 = currentSolution.solution[neighbour.machine][neighbour.startIndex];
    for (auto &node: disjunctiveGraph[job_node1]) {
        if (node->machine == neighbour.machine) node1 = node;
    }

    int job_node2 = currentSolution.solution[neighbour.machine][neighbour.endIndex];
    for (auto &node: disjunctiveGraph[job_node2]) {
        if (node->machine == neighbour.machine) node2 = node;
    }

    // execute swap move: rearrange pointers
    if (neighbour.swap == forward) {
        auto buff_mp = node1->machPredecessor.expired() ? nullptr : node1->machPredecessor.lock();
        auto buff_ms = node1->machSuccessor;
        node1->machPredecessor = node2;
        node1->machSuccessor = node2->machSuccessor;
        if (node1->machSuccessor) node1->machSuccessor->machPredecessor = node1;
        node2->machSuccessor = node1;

        if (buff_mp) buff_mp->machSuccessor = buff_ms;
        if (buff_ms) buff_ms->machPredecessor = buff_mp;
    } else if (neighbour.swap == backward){
        auto buff_mp = node2->machPredecessor.expired() ? nullptr : node2->machPredecessor.lock();
        auto buff_ms = node2->machSuccessor;
        node2->machPredecessor = node1->machPredecessor;
        node2->machSuccessor = node1;
        if (!node2->machPredecessor.expired()) node2->machPredecessor.lock()->machSuccessor = node2;
        node1->machPredecessor = node2;

        if (buff_mp) buff_mp->machSuccessor = buff_ms;
        if (buff_ms) buff_ms->machPredecessor = buff_mp;
    } else { // neighbour.swap == adjacent
        auto buff_mp = node1->machPredecessor.expired() ? nullptr : node1->machPredecessor.lock();
        node1->machSuccessor = node2->machSuccessor;
        if (node1->machSuccessor) node1->machSuccessor->machPredecessor = node1;
        node1->machPredecessor = node2;

        node2->machPredecessor = buff_mp;
        node2->machSuccessor = node1;
        if (buff_mp) buff_mp->machSuccessor = node2;
    }

    // leftshift / recalculate starting times
    // reset all longest paths to dummy end node
    vector<std::shared_ptr<Node>> start_nodes = vector<std::shared_ptr<Node>>();
    start_nodes.reserve(2 * instance.machineCount * instance.jobCount);
    for (auto &job: disjunctiveGraph) {
        if (job.front()->machPredecessor.expired()) start_nodes.emplace_back(job.front());
        for (auto &node: job) {
            node->lenToN = 0;
            node->start = 0;
        }
    }
    long long pos = 0;
    while (pos < start_nodes.size()) {
        auto &node = start_nodes[pos];
        auto &ms = node->machSuccessor;
        auto &js = node->jobSuccessor;
        auto end = node->start + node->duration;
        if (ms && ms->start < end) {
            ms->start = end;
            start_nodes.emplace_back(ms);
        }
        if (js && js->start < end) {
            js->start = end;
            start_nodes.emplace_back(js);
        }
        ++pos;
    }
    auto makespan = 0;
    for (auto &job: disjunctiveGraph) {
        makespan = std::max(makespan, job.back()->start + job.back()->duration);
    }
    currentSolution.makespan = makespan;
}

vector<vector<std::shared_ptr<Node>>> TabuSearch::generateDisjunctiveGraph() const {
    auto makespan_machine = vector<int>(instance.machineCount);
    auto sol_ptr = vector<int>(instance.machineCount);
    auto makespan_job = vector<int>(instance.jobCount);
    auto job_ptr = vector<int>(instance.jobCount);
    auto disjunctive_graph = vector<vector<std::shared_ptr<Node>>>(instance.jobCount);
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

                auto job_predecessor = disjunctive_graph[job].empty() ? null_node : disjunctive_graph[job].back();
                auto machine_predecessor = disjunctive_arcs[machine].empty() ? null_node : disjunctive_arcs[machine].back();
                auto node = std::make_shared<Node>(Node{job_predecessor, machine_predecessor, nullptr, nullptr, machine, job, start, duration, 0});
                if (job_predecessor) {
                    disjunctive_graph[job].back()->jobSuccessor = node;
                    disjunctive_graph[job].emplace_back(node);
                } else {
                    disjunctive_graph[job].emplace_back(node);
                }
                if (machine_predecessor) {
                    disjunctive_arcs[machine].back()->machSuccessor = node;
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
    return disjunctive_graph;
}