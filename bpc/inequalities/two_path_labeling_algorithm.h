// #ifdef USE_TWO_PATH_CUTS
//
// #ifndef VRP_TWO_PATH_LABELING_ALGORTHM_H
// #define VRP_TWO_PATH_LABELING_ALGORTHM_H
//
// #include "inequalities/separator_two_path.h"
// #include "problem/instance.h"
// #include "types/priority_queue.h"
// #include "types/basic_types.h"
// #include <immintrin.h>
//
// struct TwoPathLabel
// {
//     Int16 vertex;
//     Int16 cost;
//     Int16 time;
//     Int16 unreachable;
// };
// static_assert(sizeof(TwoPathLabel) == 8);
// static_assert(alignof(TwoPathLabel) == 2);
//
// struct TwoPathLabelCompare
// {
//     bool operator()(const TwoPathLabel& a, const TwoPathLabel& b) const
//     {
//         return (a.cost < b.cost) || (a.cost == b.cost && a.time > b.time);
//     }
// };
//
// class TwoPathLabelingAlgorithm
// {
//     const Instance& instance_;
//     PriorityQueue<TwoPathLabel, TwoPathLabelCompare> queue_;
//
//   public:
//     // Constructors and destructors
//     TwoPathLabelingAlgorithm() = delete;
//     TwoPathLabelingAlgorithm(const Instance& instance) : instance_(instance) {}
//     TwoPathLabelingAlgorithm(const TwoPathLabelingAlgorithm&) = delete;
//     TwoPathLabelingAlgorithm(TwoPathLabelingAlgorithm&&) = delete;
//     TwoPathLabelingAlgorithm& operator=(const TwoPathLabelingAlgorithm&) = delete;
//     TwoPathLabelingAlgorithm& operator=(TwoPathLabelingAlgorithm&&) = delete;
//     ~TwoPathLabelingAlgorithm() = default;
//
//     // Solve
//     bool solve(const TwoPathCutsVerticesSubset& vertices_in_cut);
// };
//
// #endif
//
// #endif
