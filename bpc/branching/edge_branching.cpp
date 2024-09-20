// #define PRINT_DEBUG

// #include "branching/strong_branching.h"
// #include "inequalities/robust_cut.h"
// #include "types/hash_map.h"
// #include "types/tuple.h"
// #include <cmath>
#include "branching/constraint_handler_edge_branching.h"
#include "branching/edge_branching.h"
#include "output/output.h"
#include "problem/instance.h"
#include "problem/problem.h"

// Branching rule properties
#define BRANCHRULE_NAME         "edge"
#define BRANCHRULE_DESC         "Edge branching rule"
#define BRANCHRULE_PRIORITY     50000
#define BRANCHRULE_MAXDEPTH     -1
#define BRANCHRULE_MAXBOUNDDIST 1.0

#define PREFER_BRANCH_0         (false)
// #define PSEUDOCOST_EPSILON      (1e-6)

// // Branching rule data
// struct BranchingRuleData
// {
//     HashMap<Edge, PseudocostsData> pseudocosts;
//     Vector<EdgeBranching> node_decisions;
// };
//
// // Update pseudocost for the current node
// static void update_pseudocost(
// #ifdef DEBUG
//     SCIP* scip,                           // SCIP
// #endif
//     BranchingRuleData* branchruledata,    // Branching rule data
//     SCIP_Node* current,                   // Current node
//     SCIP_Longint current_num,             // Number of current node
//     SCIP_Real current_lb                  // LP objective value of current node
// )
// {
//     // Check that the current node is not the root.
//     debug_assert(current_num > 1);
//
//     // Get branching rule data.
//     auto& node_decisions = branchruledata->node_decisions;
//     auto& current_decision = node_decisions[current_num];
//
//     // Store the dual bound of the current node.
//     debug_assert(std::isnan(current_decision.lb));
//     current_decision.lb = current_lb;
//
//     // Update the pseudocost if the current node is branched by edge.
//     const auto parent_num = SCIPnodeGetNumber(SCIPnodeGetParent(current));
//     const auto& parent_decision = node_decisions[parent_num];
//     if (!std::isnan(parent_decision.val))
//     {
//         // Print.
//         debugln("    Updating pseudocost at node {} (parent {})", current_num, parent_num);
//
//         // Compute pseudocost for the node and update the average.
//         debug_assert(!SCIPisIntegral(scip, current_decision.val));
//         const auto parent_lb = parent_decision.lb;
//         const auto current_edge = current_decision.edge;
//         auto& edge_pseudocosts = branchruledata->pseudocosts[current_edge];
//         if (current_decision.dir == BranchDirection::Forbid)
//         {
//             const auto pcost = (current_lb - parent_lb) / (current_decision.val);
//             edge_pseudocosts.down_pcost_sum += pcost;
//             edge_pseudocosts.down_ntimes++;
//
//             debugln("        Down branch: dual bound {:.2f}, current node pseudocost {:.2f}, edge pseudocost {:.2f}",
//                     current_lb,
//                     pcost,
//                     edge_pseudocosts.down_pcost_sum / edge_pseudocosts.down_ntimes);
//         }
//         else
//         {
//             const auto pcost = (current_lb - parent_lb) / (1.0 - current_decision.val);
//             edge_pseudocosts.up_pcost_sum += pcost;
//             edge_pseudocosts.up_ntimes++;
//
//             debugln("        Up branch: dual bound {:.2f}, current node pseudocost {:.2f}, edge pseudocost {:.2f}",
//                     current_lb,
//                     pcost,
//                     edge_pseudocosts.up_pcost_sum / edge_pseudocosts.up_ntimes);
//         }
//         debugln("");
//     }
// }

Pair<SCIP_Node*, SCIP_Node*> create_children_using_edge(
    SCIP* scip,        // SCIP
    const Edge edge    // Edge
)
{
    // Get the problem.
    const auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
    const auto& instance = *problem.instance;

    // Check.
#ifdef DEBUG
    {
        const auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
        const auto& instance = *problem.instance;
        debug_assert(edge.i < instance.num_customers() || edge.j < instance.num_customers());
    }
#endif

    // Check that the decision hasn't been branched on already.
#ifdef DEBUG
    {
        // Get instance.
        const auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
        const auto& instance = *problem.instance;

        // Get constraints for edge branching decisions.
        auto conshdlr = problem.edge_branching_conshdlr;
        const auto num_edge_branching_conss = SCIPconshdlrGetNConss(conshdlr);
        auto edge_branching_conss = SCIPconshdlrGetConss(conshdlr);
        debug_assert(num_edge_branching_conss == 0 || edge_branching_conss);

        // Loop through decisions in the ancestors of this node.
        for (Size idx = 0; idx < num_edge_branching_conss; ++idx)
        {
            // Get the constraint.
            auto cons = edge_branching_conss[idx];
            debug_assert(cons);

            // Check.
            release_assert(!SCIPconsIsActive(cons) || SCIPgetEdgeBranchingDecision(cons).edge != edge,
                           "Already branched on customer edge ({},{}) in an ancestor",
                           instance.vertex_name[edge.i],
                           instance.vertex_name[edge.j]);
        }
    }
#endif

    // Create children nodes.
    SCIP_Node* branch_up_node;
    SCIP_Node* branch_down_node;
    const auto score = SCIPgetLocalTransEstimate(scip);
    scip_assert(SCIPcreateChild(scip,
                                &branch_up_node,
                                1.0 - static_cast<SCIP_Real>(PREFER_BRANCH_0),
                                score + (PREFER_BRANCH_0 ? 1.0 : 0.0)));
    scip_assert(SCIPcreateChild(scip,
                                &branch_down_node,
                                static_cast<SCIP_Real>(PREFER_BRANCH_0),
                                score + (PREFER_BRANCH_0 ? 0.0 : 1.0)));
    debugln("    Creating up node {} and down node {}",
            SCIPnodeGetNumber(branch_up_node),
            SCIPnodeGetNumber(branch_down_node));

    // Create local constraints that enforce the decision in the children nodes.
    SCIP_Cons* branch_up_cons;
    SCIP_Cons* branch_down_cons;
    const auto branch_up_name = fmt::format("branch_use({},{})",
                                            instance.vertex_name[edge.i],
                                            instance.vertex_name[edge.j]);
    const auto branch_down_name = fmt::format("branch_forbid({},{})",
                                              instance.vertex_name[edge.i],
                                              instance.vertex_name[edge.j]);
    scip_assert(SCIPcreateConsEdgeBranching(scip,
                                            &branch_up_cons,
                                            branch_up_name.c_str(),
                                            branch_up_node,
                                            edge,
                                            BranchDirection::Require,
                                            true));
    scip_assert(SCIPcreateConsEdgeBranching(scip,
                                            &branch_down_cons,
                                            branch_down_name.c_str(),
                                            branch_down_node,
                                            edge,
                                            BranchDirection::Forbid,
                                            true));

    // Add the constraint to each node.
    scip_assert(SCIPaddConsNode(scip, branch_up_node, branch_up_cons, nullptr));
    scip_assert(SCIPaddConsNode(scip, branch_down_node, branch_down_cons, nullptr));

    // Decrease reference counter of the constraints.
    scip_assert(SCIPreleaseCons(scip, &branch_up_cons));
    scip_assert(SCIPreleaseCons(scip, &branch_down_cons));

    // Return.
    return {branch_down_node, branch_up_node};
}

// Pair<SCIP_Node*, SCIP_Node*> create_children_using_robust_constraint(
//     SCIP* scip,                           // SCIP
//     Problem& problem,                     // Problem
//     RobustCut&& branch_up_robust_cut,     // Constraint in the up child node
//     RobustCut&& branch_down_robust_cut    // Constraint in the down child node
// )
// {
//     // Check that the decision hasn't been branched on already.
// #ifdef DEBUG
//     {
//         const auto& robust_cuts = problem.robust_cuts;
//         for (auto& robust_cut : robust_cuts)
//         {
//             auto cons = robust_cut.cons();
//             if (cons)
//             {
//                 release_assert(!SCIPconsIsActive(cons) ||
//                                (robust_cut.name() != branch_up_robust_cut.name() &&
//                                 robust_cut.name() != branch_down_robust_cut.name()),
//                                "Already branched on {} in an ancestor",
//                                robust_cut.name());
//             }
//         }
//     }
// #endif
//
//     // Create children nodes.
//     SCIP_Node* branch_up_node;
//     SCIP_Node* branch_down_node;
//     const auto score = SCIPgetLocalTransEstimate(scip);
//     scip_assert(SCIPcreateChild(scip,
//                                 &branch_up_node,
//                                 1.0 - static_cast<SCIP_Real>(PREFER_BRANCH_0),
//                                 score + (PREFER_BRANCH_0 ? 1.0 : 0.0)));
//     scip_assert(SCIPcreateChild(scip,
//                                 &branch_down_node,
//                                 static_cast<SCIP_Real>(PREFER_BRANCH_0),
//                                 score + (PREFER_BRANCH_0 ? 0.0 : 1.0)));
//     debugln("    Creating up node {} and down node {}",
//             SCIPnodeGetNumber(branch_up_node),
//             SCIPnodeGetNumber(branch_down_node));
//
//     // Add the constraint to each node.
//     SCIP_Cons* branch_up_cons;
//     SCIP_Cons* branch_down_cons;
//     SCIP_Result result;
// #ifdef DEBUG
//     problem.update_variable_values(scip);    // add_robust_constraint() checks for variable values.
// #endif
//     std::tie(result, branch_up_cons) = problem.add_robust_constraint(scip,
//                                                                      std::move(branch_up_robust_cut),
//                                                                      branch_up_node);
//     std::tie(result, branch_down_cons) = problem.add_robust_constraint(scip,
//                                                                        std::move(branch_down_robust_cut),
//                                                                        branch_down_node);
//
//     // Return.
//     return {branch_down_node, branch_up_node};
// }
//
// #ifdef USE_BRANCH_ON_VEHICLES
// bool branch_on_number_of_vehicles(
//     SCIP* scip,                          // SCIP
//     Problem& problem,                    // Problem
//     const Instance& instance,            // Instance
//     BranchingRuleData* branchruledata    // Branching rule data
// )
// {
//     // Create output.
//     auto success = false;
//
//     // Get instance.
//     const auto nb_vertices = instance.nb_vertices();
//     const auto depot = instance.depot();
//
//     // Get variables.
//     const auto& vars = problem.vars;
//
//     // Size the number of times the edges outgoing from the depot is used.
//     SCIP_Real lhs = 0;
//     for (const auto& [var, val, path, path_length] : vars)
//     {
//         debug_assert(val == SCIPgetSolVal(scip, nullptr, var));
//         lhs += val;
//     }
//
//     // Create the branching constraints if violated.
//     if (!SCIPisIntegral(scip, lhs))
//     {
//         // Print.
//         debugln("    Branching on number of vehicles with value {:.4f}",
//                 lhs,
//                 SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
//                 SCIPgetDepth(scip));
//
//         // Size the number of outgoing edges from the depot.
//         Size count = 0;
//         for (Vertex j = 0; j < nb_vertices; ++j)
//             if (instance.is_valid(depot, j))
//             {
//                 ++count;
//             }
//
//         // Create local constraints that enforce the branching in the children nodes.
//         RobustCut branch_up_robust_cut(scip,
//                                        count,
//                                        SCIPceil(scip, lhs)
// #ifdef DEBUG
//                                      , fmt::format("branch_vehicles_geq({})", SCIPceil(scip, lhs))
// #endif
//         );
//         RobustCut branch_down_robust_cut(scip,
//                                          count,
//                                          -SCIPfloor(scip, lhs)
// #ifdef DEBUG
//                                        , fmt::format("branch_vehicles_leq({})", SCIPfloor(scip, lhs))
// #endif
//         );
//         Size idx = 0;
//         for (Vertex j = 0; j < nb_vertices; ++j)
//             if (instance.is_valid(depot, j))
//             {
//                 branch_up_robust_cut.edges(idx) = EdgeVal{depot, j, 1};
//                 branch_down_robust_cut.edges(idx) = EdgeVal{depot, j, -1};
//                 ++idx;
//             }
//         debug_assert(idx == count);
//
//         // Create the children nodes.
//         auto [branch_down_node, branch_up_node] =
//             create_children_using_robust_constraint(scip,
//                                                     problem,
//                                                     std::move(branch_up_robust_cut),
//                                                     std::move(branch_down_robust_cut));
//         debug_assert(SCIPnodeGetNumber(branch_down_node) == SCIPnodeGetNumber(branch_up_node) + 1);
//         success = true;
//
//         // Allocate space for the node decision.
//         {
//             constexpr auto nan = std::numeric_limits<SCIP_Real>::quiet_NaN();
//             auto& node_decisions = branchruledata->node_decisions;
//
//             const auto child_num = SCIPnodeGetNumber(branch_down_node);
//             if (static_cast<SCIP_Longint>(node_decisions.size()) < child_num + 1)
//             {
//                 node_decisions.resize(child_num + 1, EdgeBranching{{-1, -1}, BranchDirection::Forbid, nan, nan});
//             }
//         }
//     }
//
//     // Return.
//     return success;
// }
// #endif

Bool branch_on_edge(
    SCIP* scip,                                                     // SCIP
    // BranchingRuleData* branchruledata,                           // Branching rule dat
    const HashMap<Edge, Pair<SCIP_Real, Size>>& fractional_edges    // Edges with fractional value
)
{
    // Check.
    debug_assert(!fractional_edges.empty());

    // Get pseudocosts data.
    // const auto& pseudocosts = branchruledata->pseudocosts;

    // Select an edge to branch on.
    Edge selected_edge{-1, -1};
    // SCIP_Real selected_score = 0.0;
    Size selected_count = 0;
    SCIP_Real selected_val = 0.0;
    debugln("    Branching candidates:");
    for (const auto& [edge, pair] : fractional_edges)
    {
        // Get the edge data.
        const auto& [val, count] = pair;

        // Get the average pseudocosts.
        // const auto& edge_pseudocosts = pseudocosts.at(edge);
        // const auto edge_down_pcost = edge_pseudocosts.down_pcost_sum / edge_pseudocosts.down_ntimes;
        // const auto edge_up_pcost = edge_pseudocosts.up_pcost_sum / edge_pseudocosts.up_ntimes;
        // debug_assert(SCIPisSumGE(scip, edge_down_pcost, 0.0));
        // debug_assert(SCIPisSumGE(scip, edge_up_pcost, 0.0));

        // Compute the score.
        // const auto edge_score = std::max(edge_val * edge_down_pcost, PSEUDOCOST_EPSILON) *
        //     std::max((1.0 - edge_val) * edge_up_pcost, PSEUDOCOST_EPSILON);

        // Decide whether to accept the edge.
        // if (SCIPisGT(scip, edge_score, selected_score) ||
        //     (SCIPisEQ(scip, edge_score, selected_score) && edge_count > selected_count))
        debug_assert(!SCIPisIntegral(scip, val));
        if ((count > selected_count) ||
            (count == selected_count && std::abs(val - 0.5) < std::abs(selected_val - 0.5)))
        {
            selected_edge = edge;
            // selected_score = edge_score;
            selected_count = count;
            selected_val = val;
        }

        // Print.
        debugln("        ({},{}) with value {:.4f}, count {}",
                //, score {:.2f}, down pseudocost {:.2f}, up pseudocost {:.2f}",
                reinterpret_cast<Problem*>(SCIPgetProbData(scip))->instance->vertex_name[edge.i],
                reinterpret_cast<Problem*>(SCIPgetProbData(scip))->instance->vertex_name[edge.j],
                val,
                count/*,
                edge_score,
                edge_down_pcost,
                edge_up_pcost*/);
    }
    debugln("");
    release_assert(selected_edge.i != selected_edge.j, "Failed to select an edge for branching");

    // Print.
// #ifdef PRINT_DEBUG
//     {
//         // Get the average pseudocosts.
//         const auto& edge_pseudocosts = pseudocosts.at(selected_edge);
//         const auto edge_down_pcost = edge_pseudocosts.down_pcost_sum / edge_pseudocosts.down_ntimes;
//         const auto edge_up_pcost = edge_pseudocosts.up_pcost_sum / edge_pseudocosts.up_ntimes;
//
//         // Print.
//         debugln("    Branching on edge ({},{}) with value {:.4f}, count {}, score {:.2f}, down pseudocost {:.2f}, "
//                 "up pseudocost {:.2f} at node {}, depth {}",
//                 reinterpret_cast<Problem*>(SCIPgetProbData(scip))->instance->vertex_name[selected_edge.i],
//                 reinterpret_cast<Problem*>(SCIPgetProbData(scip))->instance->vertex_name[selected_edge.j],
//                 selected_val,
//                 selected_count,
//                 selected_score,
//                 edge_down_pcost,
//                 edge_up_pcost,
//                 SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
//                 SCIPgetDepth(scip));
//     }
// #endif

    // Create the children nodes.
    debug_assert(SCIPisGT(scip, selected_val, 0));
    debug_assert(SCIPisLT(scip, selected_val, 1));
    auto [branch_down_node, branch_up_node] = create_children_using_edge(scip, selected_edge);
    debug_assert(branch_down_node);
    debug_assert(branch_up_node);
    debug_assert(SCIPnodeGetNumber(branch_down_node) == SCIPnodeGetNumber(branch_up_node) + 1);

    // Store the decision of the children nodes.
//     constexpr auto nan = std::numeric_limits<SCIP_Real>::quiet_NaN();
//     auto& node_decisions = branchruledata->node_decisions;
//     {
//         // Get the node number.
//         const auto child_num = SCIPnodeGetNumber(branch_down_node);
//
//         // Resize.
//         if (static_cast<SCIP_Longint>(node_decisions.size()) < child_num + 1)
//         {
//             node_decisions.resize(child_num + 1, EdgeBranching{{-1, -1}, BranchDirection::Forbid, nan, nan});
//         }
//
//         // Store the decision.
//         node_decisions[child_num] = {selected_edge, BranchDirection::Forbid, selected_val, nan};
//     }
//     {
//         // Get the node number.
//         const auto child_num = SCIPnodeGetNumber(branch_up_node);
//
//         // Store the decision.
//         debug_assert(child_num < static_cast<SCIP_Longint>(node_decisions.size()));
//         node_decisions[child_num] = {selected_edge, BranchDirection::Use, selected_val, nan};
//     }

    // Return.
    return true;
}

// Branching execution method for fractional LP solutions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_BRANCHEXECLP(branchExeclpEdge)
{
    // Check.
    debug_assert(scip);
    debug_assert(branchrule);
    debug_assert(strcmp(SCIPbranchruleGetName(branchrule), BRANCHRULE_NAME) == 0);
    debug_assert(result);

    // Print.
    debug_separator();
    debugln("Branching on fractional LP at node {}, depth {}, lb {}, ub {}:",
            SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
            SCIPgetDepth(scip),
            SCIPnodeGetLowerbound(SCIPgetCurrentNode(scip)),
            SCIPgetUpperbound(scip));

    // Print paths.
#ifdef PRINT_DEBUG
    debugln("");
    print_positive_paths(scip);
    debugln("");
#endif

    // Get instance.
    auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
    // const auto& instance = *problem.instance;

    // Get branching rule data.
    // auto branchruledata = reinterpret_cast<BranchingRuleData*>(SCIPbranchruleGetData(branchrule));
    // auto& pseudocosts = branchruledata->pseudocosts;
    // auto& node_decisions = branchruledata->node_decisions;

    // Update variable values.
    problem.update_variable_values(scip);
    const auto& fractional_edges = problem.fractional_edges;

    // If at the root node, perform strong branching to compute pseudocosts. Otherwise, update pseudocosts.
//     {
//         const auto current = SCIPgetCurrentNode(scip);
//         const auto current_num = SCIPnodeGetNumber(current);
//         const auto current_lb = SCIPnodeGetLowerbound(current);
//         if (current_num == 1)
//         {
//             // Store the dual bound of the current node.
//             auto& current_decision = node_decisions[current_num];
//             debug_assert(std::isnan(current_decision.lb));
//             current_decision.lb = current_lb;
//
//             // Perform strong branching.
//             SCIP_CALL(strong_branching_for_pseudocosts(scip, pseudocosts, fractional_edges, current_lb));
//         }
//         else
//         {
//             // Update pseudocosts using the bound change of the current node.
//             update_pseudocost(
//     #ifdef DEBUG
//                               scip,
//     #endif
//                               branchruledata,
//                               current,
//                               current_num,
//                               current_lb);
//         }
//     }

    // Branch on number of vehicles.
// #ifdef USE_BRANCH_ON_VEHICLES
//     {
//         success = branch_on_number_of_vehicles(scip, problem, instance, branchruledata);
//         if (success)
//         {
//             goto BRANCHED;
//         }
//     }
// #endif

    // Print fractional edges.
#ifdef PRINT_DEBUG
    {
        debugln("    Fractional edges:");
        for (const auto& [edge, pair] : fractional_edges)
        {
            const auto& [val, count] = pair;
            debugln("        ({},{}) ({},{}) with value {:.4f}, count {}",
                    edge.i,
                    edge.j,
                    problem.instance->vertex_name[edge.i],
                    problem.instance->vertex_name[edge.j],
                    val,
                    count);
        }
        debugln("");
    }
#endif

    // Branch.
    release_assert(!fractional_edges.empty(), "No fractional edges for branching");
    const auto success = branch_on_edge(scip, fractional_edges);
    if (success)
    {
        *result = SCIP_BRANCHED;
        return SCIP_OKAY;
    }

    // Failed.
    UNREACHABLE();
}
#pragma GCC diagnostic pop

// Branching execution method for pseudosolutions (integer solutions)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_BRANCHEXECPS(branchExecpsEdge)
{
    // Check.
    debug_assert(scip);
    debug_assert(branchrule);
    debug_assert(strcmp(SCIPbranchruleGetName(branchrule), BRANCHRULE_NAME) == 0);
    debug_assert(result);

    // Print.
    debugln("Branching on pseudosolution at node {}, depth {}",
            SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
            SCIPgetDepth(scip));

    // Print paths.
#ifdef PRINT_DEBUG
    debugln("");
    print_positive_paths(scip);
    debugln("");
#endif

    // Nothing to do.
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Unused branching method
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_BRANCHEXECEXT(branchExecextEdge)
{
    UNREACHABLE();
}
#pragma GCC diagnostic pop

// // Allocate memory for data of branching rule
// static
// SCIP_DECL_BRANCHINIT(branchInitEdge)
// {
//     constexpr auto nan = std::numeric_limits<SCIP_Real>::quiet_NaN();
//
//     // Check.
//     debug_assert(scip);
//     debug_assert(branchrule);
//     debug_assert(strcmp(SCIPbranchruleGetName(branchrule), BRANCHRULE_NAME) == 0);
//
//     // Get instance.
//     auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
//     const auto& instance = *problem.instance;
//     const auto nb_vertices = instance.nb_vertices();
//
//     // Get the branching rule data.
//     auto branchruledata = reinterpret_cast<BranchingRuleData*>(SCIPbranchruleGetData(branchrule));
//
//     // Allocate memory for pseudocosts.
//     for (Vertex i = 0; i < nb_vertices; ++i)
//         for (Vertex j = 0; j < nb_vertices; ++j)
//             if (instance.is_valid(i, j))
//             {
//                 branchruledata->pseudocosts[{i, j}] = {nan, nan, 0, 0};
//             }
//
//     // Allocate memory for node decisions.
//     branchruledata->node_decisions.resize(10000, EdgeBranching{{-1, -1}, BranchDirection::Forbid, nan, nan});
//
//     // Done.
//     return SCIP_OKAY;
// }
//
// // Free memory for data of branching rule
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// static SCIP_DECL_BRANCHFREE(branchFreeEdge)
// {
//     // Get branching rule data.
//     auto branchruledata = reinterpret_cast<BranchingRuleData*>(SCIPbranchruleGetData(branchrule));
//
//     // Deallocate.
//     branchruledata->~BranchingRuleData();
//     SCIPfreeBlockMemory(scip, &branchruledata);
//
//     // Done.
//     return SCIP_OKAY;
// }
// #pragma GCC diagnostic pop

// Create the branching rule and include it in SCIP
SCIP_Retcode SCIPincludeEdgeBranchrule(
    SCIP* scip    // SCIP
)
{
    // Create branching rule data.
    // BranchingRuleData* branchruledata = nullptr;
    // SCIP_CALL(SCIPallocBlockMemory(scip, &branchruledata));
    // new (branchruledata) BranchingRuleData;

    // Include branching rule.
    SCIP_Branchrule* branchrule = nullptr;
    SCIP_CALL(SCIPincludeBranchruleBasic(scip,
                                         &branchrule,
                                         BRANCHRULE_NAME,
                                         BRANCHRULE_DESC,
                                         BRANCHRULE_PRIORITY,
                                         BRANCHRULE_MAXDEPTH,
                                         BRANCHRULE_MAXBOUNDDIST,
                                         nullptr)); // reinterpret_cast<SCIP_BranchruleData*>(branchruledata)
    debug_assert(branchrule);

    // Activate branching rule.
    SCIP_CALL(SCIPsetBranchruleExecLp(scip, branchrule, branchExeclpEdge));
    SCIP_CALL(SCIPsetBranchruleExecPs(scip, branchrule, branchExecpsEdge));
    SCIP_CALL(SCIPsetBranchruleExecExt(scip, branchrule, branchExecextEdge));
    // SCIP_CALL(SCIPsetBranchruleInit(scip, branchrule, branchInitEdge));
    // SCIP_CALL(SCIPsetBranchruleFree(scip, branchrule, branchFreeEdge));

    // Done.
    return SCIP_OKAY;
}
