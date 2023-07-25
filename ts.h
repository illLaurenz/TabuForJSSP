#ifndef HYBRID_EVO_ALGORITHM_TS_H
#define HYBRID_EVO_ALGORITHM_TS_H

#include <memory>
#include <chrono>
#include <iostream>
#include "jssp.h"

enum SwapDirection {forward, backward, adjacent};

struct Neighbour {
    vector<int> sequence;
    int machine;
    int makespan;
    int startIndex;
    int endIndex;
    SwapDirection swap;
};

struct Node {
    std::weak_ptr<Node> jobPredecessor;
    std::weak_ptr<Node> machPredecessor;
    std::shared_ptr<Node> jobSuccessor;
    std::shared_ptr<Node> machSuccessor;
    int machine;
    int job;
    int start;
    int duration;
    int lenToN;
};

#include "tabu_list.h"

class TabuSearch {
public:
    explicit TabuSearch (JSSPInstance &instance):
    instance(instance), rng(instance.getSeed()), tabuList(instance) { };

    // optimize a given solution for maxIteration iterations. mainly for memetic algorithm.
    Solution optimize_it(Solution &solution, long max_iterations);

    // standalone mode / logging on. optimize a solution for a maximum amount of seconds, regardless the time constraint
    BMResult optimize(Solution &solution, int seconds, int known_optimum=0);

private:
    // constructor fields
    JSSPInstance &instance;
    TabuList tabuList;

    // logging intermediate makespans while running in standalone mode
    vector<std::tuple<double, int>> makespanHistory;

    // initialized on starting search
    Solution currentSolution;
    Solution bestSolution;
    vector<vector<std::shared_ptr<Node>>> disjunctiveGraph;
    std::mt19937 rng;

    // counter
    std::chrono::time_point<std::chrono::system_clock> startTime;

    // tabu move methods
    bool tsMove(vector<Neighbour> &neighbourhood);

    // generate N7 like neighbourhood to a solution
    vector<Neighbour> generateNeighbourhood();

    // log new best makespan and time while running
    void logMakespan(int makespan);

    [[nodiscard]] vector<vector<std::shared_ptr<Node>>> generateDisjunctiveGraph() const;

    [[nodiscard]] vector<std::shared_ptr<Node>> findLongestPath(const vector<vector<std::shared_ptr<Node>>>& disjunctive_graph) const;

    void calcLongestPaths(vector<vector<std::shared_ptr<Node>>> &disjunctive_graph) const;

    void recursiveLongestPathCalculation(const std::shared_ptr<Node>& node) const;

    [[nodiscard]] static vector<vector<std::shared_ptr<Node>>> generateBlockList(const vector<std::shared_ptr<Node>> &longest_path) ;

    [[nodiscard]] vector<Neighbour> generateNeighboursFromBlock(const vector<std::shared_ptr<Node>> &block) const;

    Neighbour static forwardSwap(vector<int> sequence, int start_index, int u, int v, int machine, vector<std::shared_ptr<Node>> const &block);
    Neighbour static backwardSwap(vector<int> sequence, int start_index, int u, int v, int machine, vector<std::shared_ptr<Node>> const &block);

    inline static bool checkForwardSwap(std::shared_ptr<Node> const &u, std::shared_ptr<Node> const &v) {
        return !u->jobSuccessor || v->lenToN + v->duration >= u->jobSuccessor->lenToN + u->jobSuccessor->duration;
    }
    inline static bool checkBackwardSwap(std::shared_ptr<Node> const &u, std::shared_ptr<Node> const &v) {
        return v->jobPredecessor.expired() || u->start + u->duration >= v->jobPredecessor.lock()->start + v->jobPredecessor.lock()->duration;
    }
    inline static bool compNeighboursByMakespan(Neighbour const &n1, Neighbour const &n2) {
        return n1.makespan < n2.makespan;
    }
    inline static bool compNodesByEndTime(std::shared_ptr<Node> const &rhs, std::shared_ptr<Node> const &lhs) {
        return rhs->start + rhs->duration > lhs->start + lhs->duration;
    }

    void updateCurrentSolution(Neighbour &neighbour);
};


#endif //HYBRID_EVO_ALGORITHM_TS_H
