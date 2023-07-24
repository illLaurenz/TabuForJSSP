#ifndef JOB_SHOP_SCHEDULING_ILLNER_2023_JSSP_H
#define JOB_SHOP_SCHEDULING_ILLNER_2023_JSSP_H


#include <string>
#include <vector>
#include <tuple>
#include <random>

using std::vector;
using std::string;

struct Operation {
    int machine;
    int duration;
    int job;
};
// intern solution struct
struct Solution {
    vector<vector<int>> solution;
    int makespan;
};
// Solution struct containing solution history of the algorithm
struct BMResult {
    vector<vector<int>> solution;
    int makespan;
    vector<std::tuple<double,int>> history;
};

class JSSPInstance {
public:
    const vector<vector<Operation>> instance;
    const int jobCount, machineCount;
    const string filename;

    explicit JSSPInstance(string &filename): instance(readInstance(filename)), jobCount(std::get<0>(readMetrics(filename))),
                                                machineCount(std::get<1>(readMetrics(filename))),
                                                filename(filename), seed(rd()) { rng.seed(seed); };
    explicit JSSPInstance(string &filename, int seed): instance(readInstance(filename)), jobCount(std::get<0>(readMetrics(filename))),
                                             machineCount(std::get<1>(readMetrics(filename))),
                                             filename(filename), seed(seed) { rng.seed(seed); };

    [[nodiscard]] unsigned int getSeed() const { return seed; };

    // reads a solution from file
    Solution readSolution(string &filename);

    // generates a random, valid solution
    Solution generateRandomSolution();

    // calculate makespan of a solution of this instance. Will not terminate, if solution is invalid.
    int calcMakespan(vector<vector<int>> const &solution) const;

    // number of operations of this instance
    [[nodiscard]] int operationCount() const;

    // repairs any invalid solution based on a random metric and returns makespan of resulting solution
    int calcMakespanAndFixSolution(vector<vector<int>> &solution, unsigned int _seed=0) const;

private:
    std::random_device rd;
    unsigned int const seed;
    std::mt19937 rng;

    // reads an instance from file
    static vector<vector<Operation>> readInstance(string &filename);

    // read first line from file (#sequence, #machines)
    static std::tuple<int,int> readMetrics(string &filename);

    // random metric for calcMakespanAndFixSolution
    void recover_soulution(vector<vector<int>> &solution, vector<int> &sol_ptr, vector<int> &job_ptr, std::mt19937 &local_rnd) const;

    vector<int> randJobList(int size);
};


#endif //JOB_SHOP_SCHEDULING_ILLNER_2023_JSSP_H
