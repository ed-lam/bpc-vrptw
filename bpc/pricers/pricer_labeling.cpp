// #define PRINT_DEBUG

// #include "inequalities/robust_cut.h"
// #include "inequalities/subset_row_cut.h"
// #include "problem/clock.h"
#include "branching/constraint_handler_edge_branching.h"
#include "labeling/labeling_algorithm.h"
#include "output/formatting.h"
#include "output/output.h"
#include "pricers/pricer_labeling.h"
#include "problem/problem.h"
#include "problem/scip.h"
#include "types/basic_types.h"
#include "types/matrix.h"
#include <scip/cons_linear.h>
#include <scip/cons_setppc.h>

// Pricer properties
#define PRICER_NAME     "labeling"
#define PRICER_DESC     "Labeling pricer"
#define PRICER_PRIORITY 1
#define PRICER_DELAY    TRUE    // Only call pricer if all problem variables have non-negative reduced costs

static void run_labeling_pricer(
    SCIP* scip,                    // SCIP
    SCIP_Pricer* pricer,           // Pricer
    const Bool feasible_master,    // Indicates if the master problem is feasible
    SCIP_Result* result,           // Output result
    SCIP_Bool*,                    // Output flag to indicate early branching is required
    SCIP_Real* lower_bound         // Output lower bound
)
{
    // Get NaN value.
    static_assert(std::numeric_limits<Cost>::has_quiet_NaN);
    constexpr auto nan = std::numeric_limits<Cost>::quiet_NaN();

    // Check.
    debug_assert(scip);
    debug_assert(result);

    // Print.
    debug_separator();
    if (feasible_master)
    {
        debugln("Starting labeling pricer for feasible master problem at node {}, depth {}, obj {}:",
                SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
                SCIPgetDepth(scip),
                SCIPgetLPObjval(scip));
    }
    else
    {
        debugln("Starting labeling pricer for infeasible master problem at node {}, depth {}:",
                SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
                SCIPgetDepth(scip));
    }

    // Get problem.
    auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
    const auto& instance = *problem.instance;
    const auto num_customers = instance.num_customers();
    const auto num_vertices = instance.num_vertices();
    const auto depot = instance.depot();

    // Print solution.
// #ifdef PRINT_DEBUG
//     if (feasible_master)
//     {
//         debugln("");
//         print_positive_paths(scip);
//         debugln("");
//     }
// #endif

    // Get solver.
    auto& labeling_algorithm = *reinterpret_cast<LabelingAlgorithm*>(SCIPpricerGetData(pricer));

    // Create matrix of reduced costs.
    auto& reduced_cost = labeling_algorithm.get_reduced_cost_matrix();
    if (feasible_master)
    {
        reduced_cost = instance.cost;
    }
    else
    {
        reduced_cost.set(0.0);
        for (Vertex i = 0; i < num_vertices; ++i)
            for (Vertex j = 0; j < num_vertices; ++j)
                if (!instance.is_valid(i, j))
                {
                    reduced_cost(i, j) = nan;
                }
    }

    // Modify the reduced cost matrix for customer cover constraints.
    // debugln("    Dual variable values:");
    {
        const auto& customer_cover_conss = problem.customer_cover_conss;
        for (Vertex i = 0; i < num_customers; ++i)
        {
            // Get the constraint.
            auto cons = customer_cover_conss[i];
            debug_assert(cons);

            // Check that the constraint is not (locally) disabled/redundant.
            debug_assert(SCIPconsIsEnabled(cons));

            // Get the dual value.
            const auto dual = feasible_master ? SCIPgetDualsolLinear(scip, cons) : SCIPgetDualfarkasLinear(scip, cons);

            // Modify the reduced cost matrix.
            for (Vertex j = 0; j < num_vertices; ++j)
            {
                reduced_cost(i, j) -= dual;
            }

            // Print.
            debugln("        {} = {}", SCIPconsGetName(cons), dual);
        }
    }

    // Modify the reduced cost matrix for robust cuts.
//     {
//         const auto& robust_cuts = problem.robust_cuts;
//         for (const auto& cut : robust_cuts)
//         {
//             // Get the constraint/cut.
//             auto cons = cut.cons();
//             auto row = cut.row();
//             debug_assert(static_cast<bool>(cons) ^ static_cast<bool>(row));
//
//             // Get the value of the dual variable.
//             Float dual = 0.0;
//             if (cons)
//             {
//                 // Skip if the constraint is in another subtree.
//                 if (SCIPconsIsEnabled(cons))
//                 {
//                     dual = feasible_master ? SCIPgetDualsolLinear(scip, cons) : SCIPgetDualfarkasLinear(scip, cons);
//                 }
//             }
//             else
//             {
//                 debug_assert(row);
//                 dual = feasible_master ? SCIProwGetDualsol(row) : SCIProwGetDualfarkas(row);
//             }
//             debug_assert(SCIPisGE(scip, dual, 0.0));
//
//             // Modify the reduced costs.
//             if (!SCIPisZero(scip, dual))
//             {
//                 // Modify the reduced cost matrix.
//                 for (Count idx = 0; idx < cut.nb_edges(); ++idx)
//                 {
//                     const auto [i, j, coeff] = cut.edges(idx);
//                     debug_assert(coeff == 1 || coeff == -1);
//                     reduced_cost(i, j) -= coeff * dual;
//                 }
//
//                 // Print.
//                 // debugln("        {} = {}", cons ? SCIPconsGetName(cons) : SCIProwGetName(row), dual);
//                 // for (Count idx = 0; idx < cut.nb_edges(); ++idx)
//                 // {
//                 //     const auto edge = cut.edges(idx);
//                 //     debugln("        ({},{}) coeff {}",
//                 //             instance.vertex_name[edge.i],
//                 //             instance.vertex_name[edge.j],
//                 //             edge.val);
//                 // }
//                 // debugln("        RHS {}", cut.rhs());
//             }
//         }
//     }

    // Get the values of the dual variables for the subset row cuts.
// #ifdef USE_SUBSET_ROW_CUTS
//     {
//         const auto& subset_row_cuts = problem.subset_row_cuts;
//
//         Vector<ThreeVertices> subset_row_cuts_vertices;
//         Vector<Float> subset_row_cuts_duals;
//         for (const auto& cut : subset_row_cuts)
//         {
//             // Get the cut.
//             auto row = cut.row();
//             debug_assert(row);
//
//             // Get the value of the dual variable.
//             const auto dual = feasible_master ? SCIProwGetDualsol(row) : SCIProwGetDualfarkas(row);
//             debug_assert(SCIPisLE(scip, dual, 0.0));
//
//             // Store the cut in the labeling algorithm.
//             if (!SCIPisZero(scip, dual))
//             {
//                 // Store the dual.
//                 subset_row_cuts_vertices.push_back({cut.i(), cut.j(), cut.k()});
//                 subset_row_cuts_duals.push_back(dual);
//
//                 // Print.
//                 // debugln("        {} = {}", SCIProwGetName(row), dual);
//             }
//         }
//         solver.store_subset_row_duals(subset_row_cuts_vertices, subset_row_cuts_duals);
//     }
// #endif

    // Enforce the edge branching decisions.
    {
        auto edge_branching_conshdlr = problem.edge_branching_conshdlr;
        auto edge_branching_conss = SCIPconshdlrGetConss(edge_branching_conshdlr);
        const auto num_edge_branching_conss = SCIPconshdlrGetNConss(edge_branching_conshdlr);
        for (Size idx = 0; idx < num_edge_branching_conss; ++idx)
        {
            // Get the constraint.
            auto cons = edge_branching_conss[idx];
            debug_assert(cons);

            // Ignore constraints that are not active since these are not on the current active path of the search tree.
            if (!SCIPconsIsActive(cons))
            {
                continue;
            }

            // Enforce the decision.
            const auto [edge, dir] = SCIPgetEdgeBranchingDecision(cons);
            if (dir == BranchDirection::Forbid)
            {
                reduced_cost(edge.i, edge.j) = nan;
                debugln("    Disabling ({},{}) by branching", edge.i, edge.j);
            }
            else
            {
                if (edge.j != depot)
                {
                    for (Vertex i = 0; i < num_vertices; ++i)
                        if (i != edge.i)
                        {
                            reduced_cost(i, edge.j) = nan;
                            debugln("    Disabling ({},{}) by branching", i, edge.j);
                        }
                }

                if (edge.i != depot)
                {
                    for (Vertex j = 0; j < num_vertices; ++j)
                        if (j != edge.j)
                        {
                            reduced_cost(edge.i, j) = nan;
                            debugln("    Disabling ({},{}) by branching", edge.i, j);
                        }
                }
            }
        }
    }

    // Inject debug solution.
//    if (SCIPnodeGetNumber(SCIPgetCurrentNode(scip)) == 4)
//    {
//        solver.set_verbose();
//
//        static int iter = 0;
//        ++iter;
//
//        Vector<Vector<String>> paths;
//        if (iter == 1)
//        {
//            paths = {{"D0", "C0", "C8", "C8->S5", "C19", "C9", "D0"}};
//        }
//        else if (iter == 2)
//        {
//            paths = {{"D0", "C1", "C1->S4", "C21", "C22", "C11", "D0"}};
//        }
//        else if (iter == 3)
//        {
//            paths = {{"D0", "C2", "C23", "C24", "C24->S3", "C3", "C3->S3", "D0"}};
//        }
//        else
//        {
//            paths = {
//                {"D0", "C5", "D0"},
//                {"D0", "S6->C18", "C18", "C10", "C6", "C6->S6", "C17", "D0"},
//                {"D0", "S2->C7", "C7", "C16", "C16->S7", "C13", "C12", "D0"},
//                {"D0", "C4", "C4->S7", "C15", "C14", "C14->S4", "C20", "D0"},
//            };
//        }
//
//        const auto& vertex_name = instance.vertex_name;
//
//        const auto reduced_cost_copy = reduced_cost;
//        reduced_cost = std::numeric_limits<Cost>::quiet_NaN();
//        for (const auto& path_vertex_names : paths)
//        {
//            Vector<Vertex> path;
//            Vertex i, j;
//            for (size_t idx = 0; idx < path_vertex_names.size() - 1; ++idx)
//            {
//                const auto i_it = std::find(vertex_name.begin(), vertex_name.end(), path_vertex_names[idx]);
//                release_assert(i_it != vertex_name.end(), "Vertex {} not found", path_vertex_names[idx]);
//                i = i_it - vertex_name.begin();
//
//                const auto j_it = std::find(vertex_name.begin(), vertex_name.end(), path_vertex_names[idx + 1]);
//                release_assert(j_it != vertex_name.end(), "Vertex {} not found", path_vertex_names[idx + 1]);
//                j = j_it - vertex_name.begin();
//
//                release_assert(instance.is_valid(i, j),
//                               "Edge ({},{}) is invalid",
//                               path_vertex_names[idx],
//                               path_vertex_names[idx + 1]);
//
//                println("{} to {}: cost {}, time {}, discharge {}, earliest {}, latest {}",
//                        instance.vertex_name[i],
//                        instance.vertex_name[j],
//                        instance.cost(i, j),
//                        instance.service_plus_travel(i, j),
//                        instance.discharge(i, j),
//                        instance.vertex_earliest[j],
//                        instance.vertex_latest[j]);
//
//                reduced_cost(i, j) = reduced_cost_copy(i, j);
//            }
//        }
//    }

    // Check feasibility of individual paths.
//    {
//        solver.set_verbose();
//
//        Vector<Vector<String>> paths{
//            // r103_75
//            {"D0", "S1->C8", "C8", "C70", "C34", "C33", "C2", "D0"},
//            {"D0", "C49", "C32", "C64", "C64->S1", "D0"},
//            {"D0", "C27", "C50", "C65", "C19", "D0"},
//            {"D0", "C46", "C48", "C51", "D0"},
//            {"D0", "C44", "C16", "C4", "C12", "D0"},
//            {"D0", "C6", "C18", "C61", "C29", "C69", "D0"},
//            {"D0", "C58", "C60", "C15", "C59", "D0"},
//            {"D0", "C5", "C13", "C37", "C43", "C36", "D0"},
//            {"D0", "C25", "C53", "C3", "C20", "D0"},
//            {"D0", "C52", "C57", "C14", "C42", "C41", "C56", "C1", "D0"},
//            {"D0", "C0", "C31", "C62", "C9", "C68", "D0"},
//            {"D0", "C67", "C28", "C23", "C11", "D0"},
//            {"D0", "C39", "C71", "C73", "C21", "C40", "D0"},
//            {"D0", "C10", "C63", "C30", "C26", "D0"},
//            {"D0", "C55", "C22", "C74", "C72", "D0"},
//            {"D0", "C17", "C7", "C45", "C35", "C47", "D0"},
//            {"D0", "C38", "C66", "C24", "C54", "D0"},
//
//            {"D0", "C52", "C52->S0", "C17", "C6", "C62", "C29", "C69", "C69->S1", "D0"}
//        };
//
//        const auto& vertex_name = instance.vertex_name;
//
//        const auto reduced_cost_copy = reduced_cost;
//
//        for (size_t idx = 0; idx < paths.size(); ++idx)
//        {
//            reduced_cost = std::numeric_limits<Cost>::quiet_NaN();
//
//            Vector<Vertex> path;
//            Vertex i, j;
//            const auto& path_vertex_names = paths[idx];
//            for (size_t idx = 0; idx < path_vertex_names.size() - 1; ++idx)
//            {
//                const auto i_it = std::find(vertex_name.begin(), vertex_name.end(), path_vertex_names[idx]);
//                release_assert(i_it != vertex_name.end(), "Vertex {} not found", path_vertex_names[idx]);
//                i = i_it - vertex_name.begin();
//
//                const auto j_it = std::find(vertex_name.begin(), vertex_name.end(), path_vertex_names[idx + 1]);
//                release_assert(j_it != vertex_name.end(), "Vertex {} not found", path_vertex_names[idx + 1]);
//                j = j_it - vertex_name.begin();
//
//                release_assert(instance.is_valid(i, j),
//                               "Edge ({},{}) is invalid",
//                               path_vertex_names[idx],
//                               path_vertex_names[idx + 1]);
//
//                println("{} to {}: cost {}, time {}, discharge {}, earliest {}, latest {}",
//                        instance.vertex_name[i],
//                        instance.vertex_name[j],
//                        instance.cost(i, j),
//                        instance.service_plus_travel(i, j),
//                        instance.discharge(i, j),
//                        instance.vertex_earliest[j],
//                        instance.vertex_latest[j]);
//
//                path.push_back(i);
//                reduced_cost(i, j) = reduced_cost_copy(i, j);
//
//                // Get the values of the dual variables for the subset row cuts.
//#ifdef USE_SUBSET_ROW_CUTS
//                {
//                    const auto& subset_row_cuts = SCIPprobdataGetSubsetRowCuts(probdata);
//
//                    Vector<ThreeVertices> subset_row_cuts_vertices;
//                    Vector<Float> subset_row_cuts_duals;
//                    for (const auto& cut : subset_row_cuts)
//                    {
//                        // Get the cut.
//                        auto row = cut.row();
//                        debug_assert(row);
//
//                        // Get the value of the dual variable.
//                        const auto dual = feasible_master ? SCIProwGetDualsol(row) : SCIProwGetDualfarkas(row);
//                        debug_assert(SCIPisLE(scip, dual, 0.0));
//
//                        // Store the cut in the labeling algorithm.
//                        if (!SCIPisZero(scip, dual))
//                        {
//                            // Store the dual.
//                            subset_row_cuts_vertices.push_back({cut.i(), cut.j(), cut.k()});
//                            subset_row_cuts_duals.push_back(dual);
//
//                            // Print.
////                            debugln("        {} = {}", SCIProwGetName(row), dual);
//                        }
//                    }
//                    solver.store_subset_row_duals(subset_row_cuts_vertices, subset_row_cuts_duals);
//                }
//#endif
//            }
//            path.push_back(j);
//
//            println("    Checking feasibility of path {}",
//                    format_path_vertex_names(path.data(), path.size(), vertex_name));
//            solver.solve(scip, probdata, feasible_master, result, lower_bound);
//        }
//
//        std::abort();
//    }

    // Print.
// #ifdef PRINT_DEBUG
//    debugln("");
//    debugln("    Reduced costs:");
//    reduced_cost.print();
// #endif

    // Solve.
    debugln("    Starting labeling algorithm");
    labeling_algorithm.solve(scip, problem, feasible_master, result, lower_bound);
}

// Reduced cost pricing for feasible master problem
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_PRICERREDCOST(pricerLabelingRedCost)
{
    run_labeling_pricer(scip, pricer, true, result, stopearly, lowerbound);
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Farkas pricing for infeasible master problem
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_PRICERFARKAS(pricerLabelingFarkas)
{
    run_labeling_pricer(scip, pricer, false, result, nullptr, nullptr);
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Free pricer data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_PRICERFREE(pricerLabelingFree)
{
    auto labeling = reinterpret_cast<LabelingAlgorithm*>(SCIPpricerGetData(pricer));
    debug_assert(labeling);
    delete labeling;

    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Create pricer and include it in SCIP
SCIP_Retcode SCIPincludePricerLabeling(SCIP* scip, const Instance& instance)
{
    // Create labeling algorithm.
    auto labeling = new LabelingAlgorithm(instance);

    // Include pricer.
    SCIP_Pricer* pricer;
    SCIP_CALL(SCIPincludePricerBasic(scip,
                                     &pricer,
                                     PRICER_NAME,
                                     PRICER_DESC,
                                     PRICER_PRIORITY,
                                     PRICER_DELAY,
                                     pricerLabelingRedCost,
                                     pricerLabelingFarkas,
                                     reinterpret_cast<SCIP_PricerData*>(labeling)));

    // Activate pricer.
    SCIP_CALL(SCIPactivatePricer(scip, pricer));
    SCIP_CALL(SCIPsetPricerFree(scip, pricer, pricerLabelingFree));

    // Done.
    return SCIP_OKAY;
}
