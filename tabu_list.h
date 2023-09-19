#ifndef HYBRID_EVO_ALGORITHM_TABU_LIST_H
#define HYBRID_EVO_ALGORITHM_TABU_LIST_H

#include <algorithm>
#include "ts.h"

/**
 * internal struct for managing the tabu solutions in tabu search
 */
struct TabuListItem {
    int tabuTenure;
    int machine;
    int id;
    int start_index;
    int end_index;
    vector<int> sequence;
    bool operator==(TabuListItem &other) const {
        return id == other.id;
    }
    bool operator==(TabuListItem const &other) const {
        return id == other.id;
    }
};

/**
 * internal class for managing the tabu list in tabu search
 * tabu list contains sequence the block of sequence (with indices) which were deciding for the move
 */
class TabuList {
public:
    explicit TabuList(JSSPInstance &instance):
            tabuListSize(calcTabuListSize(instance)), tabuList(vector<TabuListItem>()), rng(instance.getSeed()) {};
    /**
     * checks the given solution against the tabu list.
     * @param neighbour
     * @return true if the solution is tabu
     */
    bool isTabu(Neighbour const &neighbour) {
        for (auto const &tabu: tabuList) {
            if (neighbour.machine != tabu.machine) continue;
            bool isTabu = true;
            for (auto i = tabu.start_index; i <= tabu.end_index; i++) {
                if (neighbour.sequence[i] != tabu.sequence[i]) {
                    isTabu = false;
                }
            }
            if (isTabu) return true;
        }
        return false;
    }
    /**
     * updates tabu list after a tabu move
     * @param neighbour to prohibit
     * @param bestMakespan required for tenure calculation
     * (tenure := number iterations the solution stays tabu)
     */
    void updateTabuList(Neighbour const &neighbour, int bestMakespan) {
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
        int tenture_max = std::max((neighbour.makespan - bestMakespan) / d1, d2);
        std::uniform_int_distribution<std::mt19937::result_type> dist(0,tenture_max);
        int tenure = tt + dist(rng);

        tabuList.emplace_back(TabuListItem{tenure, neighbour.machine, ++tabuId, neighbour.startIndex, neighbour.endIndex, neighbour.sequence});
    }
    /**
     * calc the size of the tabu list based on job and machine count, formula by Zhang et al.
     * @param instance
     * @return the calculated size
     */
    int calcTabuListSize(JSSPInstance &instance) {
        double min = 10 + double(instance.jobCount) / instance.machineCount;
        double max;
        if (instance.machineCount * 2 > instance.jobCount) {
            max = 1.4 * min;
        } else {
            max = 1.5 * min;
        }
        std::uniform_real_distribution<> dist(0,1);
        return std::ceil(dist(rng) * (max - min) + min);
    }
    /**
     * reset the tabu list to initial state
     */
    void reset() {
         tabuList.clear();
         tabuId = 0;
     }

private:
    vector<TabuListItem> tabuList;
    const unsigned int tabuListSize;
    std::mt19937 rng;
    int tabuId = 0;
    // constants, see Zhang et al.
    const int tt = 2, d1 = 5, d2 = 12;
};


#endif //HYBRID_EVO_ALGORITHM_TABU_LIST_H
