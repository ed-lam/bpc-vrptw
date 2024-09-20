#pragma once

#include "labeling/memory_pool.h"
#include "labeling/pareto_frontier.h"
#include "problem/debug.h"
#include "problem/instance.h"
#include "problem/problem.h"
#include "problem/scip.h"
#include "types/basic_types.h"
#include "types/priority_queue.h"
#include <deque>

// #ifdef USE_SUBSET_ROW_CUTS
// struct ThreeVertices
// {
//     Vertex i;
//     Vertex j;
//     Vertex k;
// };
// #endif

struct Label
{
#ifdef DEBUG
    UInt64 id;
#endif
    const Label* parent;
    Cost cost;
    Bool dominated : 1;
    Load load : 15;
    Time time;
    Vertex vertex;
    Byte bitsets[0];

    static const Size base_size = DEBUG_ONLY(8 + ) 8*2 + 2*3;
    static const Size padding = 2;
};
static_assert(sizeof(Label) == Label::base_size + Label::padding);

struct LabelComparison
{
    Bool operator()(const Label* const lhs, const Label* const rhs) const
    {
        return lhs->cost > rhs->cost;
    }
};

class LabelingAlgorithm
{
    // Instance
    const Instance& instance_;
    Matrix<Cost> reduced_cost_;
// #ifdef USE_SUBSET_ROW_CUTS
//     ThreeVertices* subset_row_cuts_vertices_;
//     Float* subset_row_cuts_duals_;
//     Count nb_subset_row_cuts_;
// #endif

    // Solver state
    MemoryPool storage_;
    PriorityQueue<Label*, LabelComparison> queue_;
    Vector<ParetoFrontier> pareto_frontier_;
    Cost obj_;

    // Debug
#ifdef DEBUG
    Bool verbose_;
    UInt64 next_label_id_;
#endif

  public:
    // Constructors and destructor
    LabelingAlgorithm(const Instance& instance);
    LabelingAlgorithm() = delete;
    LabelingAlgorithm(const LabelingAlgorithm&) = delete;
    LabelingAlgorithm(LabelingAlgorithm&&) = delete;
    LabelingAlgorithm& operator=(const LabelingAlgorithm&) = delete;
    LabelingAlgorithm& operator=(LabelingAlgorithm&&) = delete;
    ~LabelingAlgorithm() = default;

    // Solve
    inline auto& get_reduced_cost_matrix() { return reduced_cost_; }
// #ifdef USE_SUBSET_ROW_CUTS
//     void store_subset_row_duals(const Vector<ThreeVertices>& subset_row_cuts_vertices, const Vector<Float>& subset_row_cuts_duals);
// #endif
    void solve(SCIP* scip, Problem& problem, const Bool feasible_master, SCIP_Result* result, Cost* lower_bound);
//     inline auto obj() const { return obj_; }

    // Debug
#ifdef DEBUG
    void set_verbose(const bool on = true);
#endif

  private:
    // Solve
    Size unreachable_size() const
    {
        const auto bitset_size = (instance_.num_customers() + CHAR_BIT - 1) / CHAR_BIT;
        return bitset_size;
    }
    Size label_size() const
    {
        const auto bitset_size = unreachable_size();
        const auto label_size = Label::base_size + bitset_size;
        const auto size = ((label_size + 7) & (-8)); // Round up to next multiple of 8
        return size;
    }
    void create_source_label();
    Label* extend_to_customer(const Label* const __restrict current, const Vertex j);
    Label* extend_to_sink(const Label* const __restrict current);
};
