#ifndef HYBRID_EVO_ALGORITHM_TS_H
#define HYBRID_EVO_ALGORITHM_TS_H

#include "jssp.h"
#include <memory>
#include <chrono>

struct TabuListItem {
    int tabuTenure;
    int machine;
    int id;
    vector<int> indices;
    vector<int> jobs;
    bool operator==(TabuListItem &other) const {
        return id == other.id;
    }
    bool operator==(TabuListItem const &other) const {
      return id == other.id;
    }
};

struct Neighbour {
    vector<int> sequence;
    int machine;
    int makespan;
};

struct Node {
    std::weak_ptr<Node> job_predecessor;
    std::weak_ptr<Node> mach_predecessor;
    std::shared_ptr<Node> job_successor;
    std::shared_ptr<Node> mach_successor;
    int machine;
    int job;
    int start;
    int duration;
    int len_to_n;
};

class TabuSearch {
public:
    explicit TabuSearch (JSSPInstance &instance):
    instance(instance), tabuListSize(calcTabuListSize(instance)), rng(instance.getSeed()) { };

    // optimize a given solution for maxIteration iterations. mainly for memetic algorithm.
    Solution optimize_it(Solution &solution, long maxIterations);

    // standalone mode / logging on. optimize a solution for a maximum amount of seconds, regardless the time constraint
    BMResult optimize(Solution &solution, int seconds, int known_optimum=0);

private:
    // constructor fields
    JSSPInstance &instance;

    // auto generated in constructor
    const int tabuListSize;

    // logging intermediate makespans while running in standalone mode
    vector<std::tuple<double, int>> makespan_history;

    // initialized on starting search
    Solution currentSolution;
    Solution currentBestSolution;
    int new_makespan = 0;
    vector<vector<std::shared_ptr<Node>>> disjunctive_graph;
    vector<vector<std::shared_ptr<Node>>> new_disjunctive_graph;
    vector<TabuListItem> tabuList;
    std::mt19937 rng;

    // counter
    int tabuId = 0;
    std::chrono::time_point<std::chrono::system_clock> t_start;

    // helper function for constructor
    int calcTabuListSize(JSSPInstance const &instance);

    // tabu move methods
    bool tsMove(vector<Neighbour> &neighbourhood);
    void updateTabuList(Neighbour const &neighbour);
    bool isTabu(Neighbour const &neighbour);

    // generate N7 like neighbourhood to a solution
    vector<Neighbour> generateNeighbourhood();

    // log new best makespan and time while running
    void logMakespan(int makespan);

    [[nodiscard]] vector<vector<std::shared_ptr<Node>>> generateDisjunctiveGraph() const;

    [[nodiscard]] vector<std::shared_ptr<Node>> findLongestPath(const vector<vector<std::shared_ptr<Node>>>& disjunctive_graph) const;

    vector<std::shared_ptr<Node>> recursiveLPSearch(vector<std::shared_ptr<Node>> &longest_path, int sum_durations) const;

    void calcLongestPaths(vector<vector<std::shared_ptr<Node>>> &disjunctive_graph) const;

    void recursiveLPCalculation(const std::shared_ptr<Node>& node) const;

    [[nodiscard]] static vector<vector<std::shared_ptr<Node>>> generateBlockList(const vector<std::shared_ptr<Node>> &longest_path) ;

    [[nodiscard]] vector<Neighbour> generateNeighboursFromBlock(const vector<std::shared_ptr<Node>> &block) const;

    Neighbour static forwardSwap(vector<int> sequence, int start_index, int u, int v, int machine, vector<std::shared_ptr<Node>> const &block);
    Neighbour static backwardSwap(vector<int> sequence, int start_index, int u, int v, int machine, vector<std::shared_ptr<Node>> const &block);

    inline static bool checkForwardSwap(std::shared_ptr<Node> const &u, std::shared_ptr<Node> const &v) {
        return u->job_successor == nullptr || v->len_to_n + v->duration >= u->job_successor->len_to_n + u->job_successor->duration;
    }
    inline static bool checkBackwardSwap(std::shared_ptr<Node> const &u, std::shared_ptr<Node> const &v) {
        return v->job_predecessor.expired() || u->start + u->duration >= v->job_predecessor.lock()->start + v->job_predecessor.lock()->duration;
    }
    inline static bool compareNeighbours(Neighbour const &n1, Neighbour const &n2) {
        return n1.makespan < n2.makespan;
    }
    inline static bool sort_end_nodes(std::shared_ptr<Node> const &rhs, std::shared_ptr<Node> const &lhs) {
        return rhs->start + rhs->duration > lhs->start + lhs->duration;
    }

    void updateCurrentSolution(Neighbour &neighbour);
    void printDGraph(vector<vector<std::shared_ptr<Node>>> &d_graph);
};


#endif //HYBRID_EVO_ALGORITHM_TS_H
