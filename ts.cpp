#include <cmath>
#include "ts.h"
#include <algorithm>
#include <iostream>
#include <queue>

void TabuSearch::logMakespan(int makespan) {
    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    makespan_history.emplace_back(std::tuple{elapsed_seconds.count(), makespan});
}

// main method: resets algo vars and starts loop
Solution TabuSearch::optimize_it(Solution &solution, long maxIterations) {
    tabuList = vector<TabuListItem>();
    tabuId = 0;
    currentSolution = {solution.solution, solution.makespan};
    currentBestSolution = {solution.solution, solution.makespan};
    disjunctive_graph = generateDisjunctiveGraph();
    //new_disjunctive_graph = disjunctive_graph;
    long iteration = 0;

    while (iteration++ <= maxIterations) {
        auto neighbourhood = generateNeighbourhood();
        tsMove(neighbourhood);
        if (currentSolution.makespan < currentBestSolution.makespan) {
            currentBestSolution = currentSolution;
        }
    }
    return Solution{currentBestSolution.solution, currentBestSolution.makespan};
}

// main method: resets algo vars and starts loop
BMResult TabuSearch::optimize(Solution &solution, int seconds, int known_optimum) {
    t_start = std::chrono::system_clock::now();
    makespan_history = vector<std::tuple<double,int>>();

    tabuList = vector<TabuListItem>();
    tabuId = 0;
    currentSolution = {solution.solution, solution.makespan};
    currentBestSolution = {solution.solution, solution.makespan};
    logMakespan(currentBestSolution.makespan);

    std::chrono::duration<double> elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    // main loop
    while (elapsed_seconds.count() < seconds && known_optimum != currentBestSolution.makespan) {
        auto neighbourhood = generateNeighbourhood();
        tsMove(neighbourhood);
        if (currentSolution.makespan < currentBestSolution.makespan) {
            currentBestSolution = currentSolution;
            logMakespan(currentBestSolution.makespan);
        }
        elapsed_seconds = (std::chrono::system_clock::now() - t_start);
    }
    return BMResult{currentBestSolution.solution, currentBestSolution.makespan, makespan_history};
}

// calc the size of the tabu list based on job and machine count
int TabuSearch::calcTabuListSize(const JSSPInstance &instance) {
    double min = 10 + instance.jobCount / instance.machineCount;
    double max;
    if (instance.machineCount * 2 > instance.jobCount) {
        max = 1.4 * min;
    } else {
        max = 1.5 * min;
    }
    std::uniform_real_distribution<> dist(0,1);
    return std::ceil(dist(rng) * (max - min) + min);
}

// generates N7 like neighbourhood, with the difference that cyclic solutions are accepted and fixed
// more computational overhead than in the paper
vector<Neighbour> TabuSearch::generateNeighbourhood() {
    //disjunctive_graph = generateDisjunctiveGraph(); // -
    calcLongestPaths(disjunctive_graph);
    //calcLongestPaths(new_disjunctive_graph); // -
    //printDGraph(new_disjunctive_graph); // -
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
                if (job_predecessor != nullptr) {
                    _disjunctive_graph[job].back()->job_successor = node;
                    _disjunctive_graph[job].back() = node;
                } else {
                    _disjunctive_graph[job].emplace_back(node);
                    _disjunctive_graph[job].emplace_back(node);
                }
                if (machine_predecessor != nullptr) {
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
    for (auto const &job: d_graph) {
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
    auto longest_path = vector<std::shared_ptr<Node>>();
    for (auto &job: d_graph) {
        if (job.front()->len_to_n  + job.front()->duration == currentSolution.makespan) {
            longest_path.emplace_back(job.front());
            return recursiveLPSearch(longest_path, job.front()->duration);
        }
    }
    return {}; // tidy return, is unreachable for feasible solutions
}

vector<std::shared_ptr<Node>> TabuSearch::recursiveLPSearch(vector<std::shared_ptr<Node>> &longest_path, int sum_durations) const{
    auto node = longest_path.back();
    if (node->job_successor != nullptr && node->job_successor->len_to_n + node->job_successor->duration + sum_durations == currentSolution.makespan) {
        longest_path.emplace_back(node->job_successor);
        return recursiveLPSearch(longest_path, sum_durations + node->job_successor->duration);
    } else if (node->mach_successor != nullptr) {
        longest_path.emplace_back(node->mach_successor);
        return recursiveLPSearch(longest_path, sum_durations + node->mach_successor->duration);
    } else {
        return longest_path;
    }
}

vector<vector<std::shared_ptr<Node>>> TabuSearch::generateBlockList(vector<std::shared_ptr<Node>> const &longest_path) {
    auto block_list = vector<vector<std::shared_ptr<Node>>>();
    auto block = vector<std::shared_ptr<Node>>();
    for (auto const &node: longest_path) {
        if (block.empty()) {
            block.emplace_back(node);
        } else if (node->machine == block.back()->machine) {
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
    int start_index, end_index;
    int machine_no = block.front()->machine;
    auto const &machine_seq = currentSolution.solution[machine_no];
    for (int i = 0; i < machine_seq.size(); i++) {
        if (machine_seq[i] == block.front()->job) {
            start_index = i;
            break;
        }
    }
    if (block.size() == 2) {
        if (checkForwardSwap(block.front(), block.back()) || checkBackwardSwap(block.front(), block.back())) {
            neighbours.emplace_back(forwardSwap(machine_seq, start_index, 0, 1, machine_no, block));
        }
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
        int js_u = block[u]->job_successor == nullptr ? 0 : block[u]->job_successor->len_to_n + block[u]->job_successor->duration;
        int ms_v = block[v]->mach_successor == nullptr ? 0 : block[v]->mach_successor->len_to_n + block[v]->mach_successor->duration;
        len_from_i[0] = std::max(js_u, ms_v) + block[u]->duration;
    }
    {
        int js_v = block[v]->job_successor == nullptr ? 0 : block[v]->job_successor->len_to_n + block[v]->job_successor->duration;
        int y_u = len_from_i[0];
        len_from_i.back() = std::max(js_v, y_u) + block[v]->duration;
    }
    for (int w = size - 2; w > 0; w--) {
        int js_w = block[w]->job_successor == nullptr ? 0 : block[w]->job_successor->len_to_n + block[w]->job_successor->duration;
        int ms_w = len_from_i[w + 1];
        len_from_i[w] = std::max(js_w, ms_w) + block[w]->duration;
    }

    int approx_makespan = 0;
    for (int i = 0; i < size; i++) {
        approx_makespan = std::max(approx_makespan, len_to_i[i] + len_from_i[i]);
    }

    return {sequence, machine, approx_makespan};
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
        int js_l = block[l]->job_successor == nullptr ? 0 : block[l]->job_successor->len_to_n + block[l]->job_successor->duration;
        int ms_v = block[v]->mach_successor == nullptr ? 0 : block[v]->mach_successor->len_to_n + block[v]->mach_successor->duration;
        len_from_i[l] = std::max(js_l, ms_v) + block[l]->duration;
    }
    for (int w = size - 3; w >= 0; w--) {
        int js_w = block[w]->job_successor == nullptr ? 0 : block[w]->job_successor->len_to_n + block[w]->job_successor->duration;
        int y_ms = len_from_i[w + 1];
        len_from_i[w] = std::max(js_w, y_ms) + block[w]->duration;
    }
    {
        int js_v = block[v]->job_successor == nullptr ? 0 : block[v]->job_successor->len_to_n + block[v]->job_successor->duration;
        int y_u = len_from_i[0];
        len_from_i.back() = std::max(js_v, y_u) + block[v]->duration;
    }

    int approx_makespan = 0;
    for (int i = 0; i < size; i++) {
        approx_makespan = std::max(approx_makespan, len_to_i[i] + len_from_i[i]);
    }
    return {sequence, machine, approx_makespan};
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
        if (neighbour.makespan < currentBestSolution.makespan) {
            // aspiration
            auto neighbour_solution = currentSolution; // -
            neighbour_solution.solution[neighbour.machine] = neighbour.sequence; // -
            int exact_makespan = instance.calcMakespan(neighbour_solution.solution); // -
            if (exact_makespan >= currentBestSolution.makespan && isTabu(neighbour)) continue; // -

            neighbour_solution.makespan = exact_makespan; // -
            currentSolution = neighbour_solution; // -
            updateCurrentSolution(neighbour); // +
            updateTabuList(neighbour);
            return true;
        } else if (!isTabu(neighbour)) {
            // best non tabu
            auto neighbour_solution = currentSolution; // -
            neighbour_solution.solution[neighbour.machine] = neighbour.sequence; // -
            neighbour_solution.makespan = instance.calcMakespan(neighbour_solution.solution); // -
            updateCurrentSolution(neighbour); // +
            updateTabuList(neighbour);
            currentSolution = neighbour_solution; // -
            return false;
        }
    }
    // chose random, if all tabu
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,neighbourhood.size() - 1);
    auto randIndex = dist(rng);
    auto &neighbour = neighbourhood[randIndex];
    updateCurrentSolution(neighbour); // +
    updateTabuList(neighbour);

    auto neighbour_solution = currentSolution; // -
    neighbour_solution.solution[neighbour.machine] = neighbour.sequence; // -
    neighbour_solution.makespan = instance.calcMakespan(neighbour_solution.solution); // -
    currentSolution = neighbour_solution; // -
    return false;
}

void TabuSearch::updateCurrentSolution(Neighbour &neighbour) {
    // old: neighbour_solution.solution[neighbour.machine] = neighbour.sequence;
    std::shared_ptr<Node> oldMachine;
    for (auto &job: disjunctive_graph) {
        std::shared_ptr<Node> * node = &job.front();
        while ((*node) != nullptr) {
            if ((*node)->machine == neighbour.machine && (*node)->mach_predecessor.expired()) {
                oldMachine = (*node);
            }
            node = &(*node)->job_successor;
        }
    }
    auto null_node = std::shared_ptr<Node>();
    auto &last = null_node;
    vector<std::shared_ptr<Node>> newNodes = vector<std::shared_ptr<Node>>();
    for (auto jobNo: neighbour.sequence) {
        last = std::make_shared<Node>(Node{null_node, last, nullptr, nullptr, neighbour.machine, jobNo, -1, -1, 0});
        newNodes.emplace_back(last);
    }
    auto *oldNode = &oldMachine;
    while ((*oldNode) != nullptr) {
        for (auto &newNode: newNodes) {
            if (newNode->job == (*oldNode)->job) {
                newNode->duration = (*oldNode)->duration;
                newNode->job_predecessor = (*oldNode)->job_predecessor;
                newNode->job_successor = (*oldNode)->job_successor;
                if (!(*oldNode)->job_predecessor.expired()) (*oldNode)->job_predecessor.lock()->job_successor = newNode;
                else disjunctive_graph[newNode->job][0] = newNode;
                if ((*oldNode)->job_successor != nullptr) (*oldNode)->job_successor->job_predecessor = newNode;
                else disjunctive_graph[newNode->job][1] = newNode;
                break;
            }
        }
        oldNode = &(*oldNode)->mach_successor;
    }
    int mach_time = 0;
    for (auto &newNode: newNodes) {
        int start = mach_time;
        if (!newNode->job_predecessor.expired()) start = std::max(start, newNode->job_predecessor.lock()->start + newNode->job_predecessor.lock()->duration);
        if (!newNode->mach_predecessor.expired()) start = std::max(start, newNode->mach_predecessor.lock()->start + newNode->mach_predecessor.lock()->duration);
        newNode->start = start;
    }

    // TODO: implement len to n cascade instead of setting 0 and calcLongestPaths
    for (auto &job: disjunctive_graph) {
        std::shared_ptr<Node> *node = &job.front();
        while ((*node) != nullptr) {
            (*node)->len_to_n = 0;
            node = &(*node)->job_successor;
        }
    }
    null_node = std::shared_ptr<Node>();
    std::shared_ptr<Node> *p_last = &null_node;
    for (auto it = newNodes.rbegin(); it != newNodes.rend(); it++) {
        (*it)->mach_successor = (*p_last);
        p_last = &(*it);
    }

    // makespan / leftshift
    // reset starting times
    vector<std::shared_ptr<Node> *> startNodes = vector<std::shared_ptr<Node> *>();
    startNodes.reserve(2 * instance.machineCount * instance.jobCount);
    for (auto &job: disjunctive_graph) {
        auto *node = &job.front();
        if ((*node)->mach_predecessor.expired()) startNodes.emplace_back(node);
        while ((*node) != nullptr) {
            (*node)->start = 0;
            node = &(*node)->job_successor;
        }
    }
    for (auto & node: startNodes) {
        recursiveStartTiming(*node);
    }
    /*long long pos = 0;
    while (pos < startNodes.size()) {
        auto node = *startNodes[pos];
        if (node->mach_successor != nullptr) {
            auto *ms = &node->mach_successor;
            (*ms)->start = std::max(node->start + node->duration, (*ms)->start);
            startNodes.emplace_back(ms);
        }
        if (node->job_successor != nullptr) {
            auto *js = &node->job_successor;
            (*js)->start = std::max(node->start + node->duration, (*js)->start);
            startNodes.emplace_back(js);
        }
        ++pos;
    }*/
    auto makespan = 0;
    for (auto &job: disjunctive_graph) {
        makespan = std::max(makespan, job.back()->start + job.back()->duration);
    }
    new_makespan = makespan;
}

void TabuSearch::recursiveStartTiming(std::shared_ptr<Node> const &node) const {
    if (node->mach_successor != nullptr) {
        auto ms = node->mach_successor;
        if (ms->start < node->start + node->duration) {
            ms->start = node->start + node->duration;
            recursiveStartTiming(ms);
        }
    }
    if (node->job_successor != nullptr) {
        auto js = node->job_successor;
        if (js->start < node->start + node->duration) {
            js->start = node->start + node->duration;
            recursiveStartTiming(js);
        }
    }
}

// checks the given solution against the tabu list.
// tabu list contains jobs the block of jobs (with indices) which were deciding for the move
bool TabuSearch::isTabu(Neighbour const &neighbour) {
    for (auto const &tabu: tabuList) {
        if (neighbour.machine != tabu.machine) continue;
        int index = 0;
        for (int i = 0; i < tabu.indices.size(); i++) {
            index = tabu.indices[0] + i;
            if (neighbour.sequence[index] != tabu.jobs[i]) {
                index = 0;
                break;
            }
        }
        if (index != 0) {
            return true;
        }
    }
    return false;
}

// updates tabu list after a tabu move
void TabuSearch::updateTabuList(const Neighbour &neighbour) {
    TabuListItem smallest = TabuListItem{INT32_MAX};
    int next = 0;
    while (next < tabuList.size()) {
        --tabuList[next].tabuTenure;
        if (tabuList[next].tabuTenure <= 0) {
            auto item = tabuList[next];
            tabuList.erase(tabuList.begin() + next);
            continue;
        } else if (tabuList[next].tabuTenure < smallest.tabuTenure){
            smallest = tabuList[next];
        }
        ++next;
    }
    if (tabuList.size() >= tabuListSize) {
        auto pos = std::find(tabuList.begin(), tabuList.end(), smallest);
        tabuList.erase(pos);
    }
    // constants
    const int tt = 2, d1 = 5, d2 = 12;
    int tenture_max = std::max((neighbour.makespan - currentBestSolution.makespan) / d1, d2);
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,tenture_max);
    int tenure = tt + dist(rng);
    vector<int> indices, jobs;
    // save difference on block machine as TabuListItem
    int index1 = -1, index2 = -1;
    for (int i = 0; i < neighbour.sequence.size(); i++) {
        auto neighbour_op = neighbour.sequence[i];
        auto sol_op = currentSolution.solution[neighbour.machine][i];
        if (neighbour_op != sol_op && index1 == -1) {
            index1 = i;
            index2 = i;
        } else if (neighbour_op != sol_op) {
            index2 = i;
        }
    }
    for (; index1 <= index2; index1++) {
        indices.emplace_back(index1);
        jobs.emplace_back(neighbour.sequence[index1]);
    }

   tabuList.emplace_back(TabuListItem{tenure, neighbour.machine, ++tabuId, indices, jobs});
}

void TabuSearch::printDGraph(vector<vector<std::shared_ptr<Node>>> &d_graph) {
    vector<vector<int>> solution = vector<vector<int>>(instance.machineCount);
    for (auto &job: d_graph) {
        auto *node = &job.front();
        while ((*node) != nullptr) {
            if ((*node)->mach_predecessor.expired()) {
                auto *mnode = node;
                while ((*mnode) != nullptr) {
                    solution[(*mnode)->machine].emplace_back((*mnode)->len_to_n);
                    mnode = &(*mnode)->mach_successor;
                }
            }
            node = &(*node)->job_successor;
        }
    }
    vector<vector<int>> solution2 = vector<vector<int>>(instance.machineCount);
    for (auto &job: disjunctive_graph) {
        auto *node = &job.front();
        while ((*node) != nullptr) {
            if ((*node)->mach_predecessor.expired()) {
                auto *mnode = node;
                while ((*mnode) != nullptr) {
                    solution2[(*mnode)->machine].emplace_back((*mnode)->len_to_n);
                    mnode = &(*mnode)->mach_successor;
                }
            }
            node = &(*node)->job_successor;
        }
    }
    if (solution != solution2) {
        for (auto i = 0; i < solution.size(); i++) {
            for (auto job: solution[i]) {
                std::cout << job << "\t";
            }
            std::cout << std::endl;
            for (auto job: solution2[i]) {
                std::cout << job << "\t";
            }
            std::cout << std::endl;
            std::cout << "-" << std::endl;
        }
        std::cout << "---------------------------------------------" << std::endl;
    }
}
