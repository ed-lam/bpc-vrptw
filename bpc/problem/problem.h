#pragma once

// #include "inequalities/robust_cut.h"
// #include "inequalities/separator_two_path.h"
// #include "inequalities/subset_row_cut.h"
// #include "inequalities/two_path_labeling_algorithm.h"
#include "problem/instance.h"
#include "problem/scip.h"
#include "types/basic_types.h"
#include "types/edge.h"
#include "types/hash_map.h"
#include "types/pointers.h"
#include "types/tuple.h"
#include "types/vector.h"
#include <random>

// struct CVRPSEP;

struct PathVariable
{
    SCIP_Var* var = nullptr;
    SCIP_Real val = 0.0;
    Vector<Vertex> path;
};

// Problem
struct Problem
{
    // Instance data
    const SharedPtr<Instance> instance;                                      // Instance
    // std::mt19937 rng;                                                        // Random number generator

    // Variables
    Vector<PathVariable> vars;                                               // Array of variables
    HashMap<Edge, Pair<SCIP_Real, Size>> fractional_edges;                   // Edges with fractional value

    // Constraints
    Vector<SCIP_Cons*> customer_cover_conss;                                 // Each customer must be visited
//     Vector<RobustCut> robust_cuts;                                           // Robust constraints and cuts
// #ifdef USE_SUBSET_ROW_CUTS
//     Vector<SubsetRowCut> subset_row_cuts;                                    // Subset row cuts
// #endif
// #ifdef USE_ROUNDED_CAPACITY_CUTS
//     SharedPtr<CVRPSEP> cvrpsep;                                              // CVRPSEP package
// #endif
// #ifdef USE_TWO_PATH_CUTS
//     SharedPtr<HashMap<TwoPathCutsVerticesSubset, Bool>> one_path_checked;    // Sets of vertices previously checked for one-path infeasibility
//     SharedPtr<TwoPathLabelingAlgorithm> two_path_labeling_algorithm;         // TSPTW solver
// #endif

    // Constraint handlers for branching
    SCIP_Conshdlr* edge_branching_conshdlr;                                  // Constraint handler for edge branching

  protected:
    // Constructors and destructor
    Problem(SCIP* scip, const SharedPtr<Instance>& instance);
    Problem() = delete;
    Problem(const Problem&) = default;
    Problem(Problem&&) = delete;
    Problem& operator=(const Problem&) = delete;
    Problem& operator=(Problem&&) = delete;
    ~Problem() = default;

  public:
    // Create and delete problem
    static void create(SCIP* scip, const SharedPtr<Instance>& instance);
    Problem* transform(SCIP* scip) const;
    void finish_solve(SCIP* scip);
    void free(SCIP* scip);

    // Add variables
    void add_priced_var(SCIP* scip, Vector<Vertex>&& path);

    // Add constraints
//     Pair<SCIP_Result, SCIP_Cons*> add_robust_constraint(
//         SCIP* scip,         // SCIP
//         RobustCut&& cut,    // Data for the cut
//         SCIP_Node* node     // Add the cut to the subtree under this node only
//     );
//     Pair<SCIP_Result, SCIP_Row*> add_robust_cut(
//         SCIP* scip,         // SCIP
//         RobustCut&& cut,    // Data for the cut
//         SCIP_Sepa* sepa     // Separator adding the cut
//     );
// #ifdef USE_SUBSET_ROW_CUTS
//     Pair<SCIP_Result, SCIP_Row*> add_subset_row_cut(
//         SCIP* scip,            // SCIP
//         SubsetRowCut&& cut,    // Data for the cut
//         SCIP_Sepa* sepa        // Separator adding the cut
//     );
// #endif

    // Update values of variables
    void update_variable_values(SCIP* scip);

    // Debug
#ifdef DEBUG
    void write_master(SCIP* scip);
#endif
};
