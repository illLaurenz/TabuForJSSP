#ifndef HYBRID_EVO_ALGORITHM_HEURISTICS_H
#define HYBRID_EVO_ALGORITHM_HEURISTICS_H

#include <memory>
#include "jssp.h"

struct MNode {
    MNode(unsigned int _job, unsigned int _duration) : job(_job), duration(_duration) {};
    std::shared_ptr<MNode> next;
    std::shared_ptr<MNode> previous;
    unsigned int job;
    unsigned int duration;
    unsigned int start;
    unsigned int end() const { return start + duration; };
};

class Machine {
private:
    std::shared_ptr<MNode> firstOp;

public:
    unsigned int insert(const std::shared_ptr<MNode>& operation, unsigned int min_time);
    vector<int> getSequence();
};

class Heuristics {
public:
    // creates a random semi active solution
    static vector<vector<int>> random(JSSPInstance &instance);
};


#endif //HYBRID_EVO_ALGORITHM_HEURISTICS_H
