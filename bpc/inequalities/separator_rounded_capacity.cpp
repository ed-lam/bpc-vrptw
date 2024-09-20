// #ifdef USE_ROUNDED_CAPACITY_CUTS
//
// // #define PRINT_DEBUG
//
// #include "inequalities/cvrpsep.h"
// #include "inequalities/robust_cut.h"
// #include "inequalities/separator_rounded_capacity.h"
// #include "output/formatting.h"
// #include "problem/clock.h"
// #include "problem/instance.h"
// #include "problem/problem.h"
// #include "types/edge.h"
// #include <cvrpsep/capsep.h>
//
// #define SEPA_NAME         "rounded_capacity"
// #define SEPA_DESC         "Separator for rounded capacity cuts"
// #define SEPA_PRIORITY     3                                        // priority of the constraint handler for separation
// #define SEPA_FREQ         1                                        // frequency for separating cuts; zero means to separate only in the root node
// #define SEPA_MAXBOUNDDIST 1.0
// #define SEPA_USESSUBSCIP  FALSE                                    // does the separator use a secondary SCIP instance?
// #define SEPA_DELAY        FALSE                                    // should separation method be delayed, if other separators found cuts?
//
// SCIP_RETCODE rounded_capacity_create_cut(
//     SCIP* scip,                        // SCIP
//     Problem& problem,                  // Problem
//     const Instance& instance,          // Instance
//     SCIP_Sepa* sepa,                   // Separator
//     const Vector<Vertex>& vertices,    // Vertices in the cut
//     const Float rhs,                   // RHS
//     SCIP_Result* result                // Output result
// )
// {
//     // Create constraint name.
// #ifdef DEBUG
//     auto name = fmt::format("rounded_capacity({})",
//                             format_path_with_names(vertices.data(), vertices.size(), instance.vertex_name, ","));
// #endif
//
//     // Count the number of edges in the cut.
//     Count count = 0;
//     for (const auto i : vertices)
//         for (const auto j : vertices)
//             if (instance.is_valid(i, j))
//             {
//                 ++count;
//             }
//
//     // Create constraint.
//     RobustCut cut(scip,
//                   count,
//                   -rhs
// #ifdef DEBUG
//                 , std::move(name)
// #endif
//     );
//     Count idx = 0;
//     for (const auto i : vertices)
//         for (const auto j : vertices)
//             if (instance.is_valid(i, j))
//             {
//                 cut.edges(idx) = EdgeVal{i, j, -1};
//                 ++idx;
//             }
//     debug_assert(idx == count);
//     SCIP_Row* row;
//     std::tie(*result, row) = problem.add_robust_cut(scip, std::move(cut), sepa);
//     debug_assert(row);
//
//     // Done.
//     return SCIP_OKAY;
// }
//
// // Separator
// static
// SCIP_RETCODE rounded_capacity_separate(
//     SCIP* scip,            // SCIP
//     SCIP_Sepa* sepa,       // Separator
//     SCIP_RESULT* result    // Pointer to store the result of the separation call
// )
// {
//     // Print.
//     debugln("Starting separator for rounded capacity cuts at node {}, depth {}, obj {}:",
//             SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
//             SCIPgetDepth(scip),
//             SCIPgetLPObjval(scip));
//
//     // Print solution.
// #ifdef PRINT_DEBUG
//     print_positive_paths(scip);
// #endif
//
//     // Start timer.
// #ifdef PRINT_DEBUG
//     const auto start_time = get_clock(scip);
// #endif
//
//     // Get problem.
//     auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
//     const auto& instance = *problem.instance;
//     const auto nb_customers = instance.nb_customers();
//
//     // Get the CVRPSEP package.
//     auto& cvrpsep = *problem.cvrpsep;
//
//     // Update variable values.
//     problem.update_variable_values(scip);
//
//     // Get the edges with positive value.
//     HashMap<Edge, Float> edge_vals;
//     {
//         const auto& vars = problem.vars;
//         for (const auto& [var, val, path, path_length] : vars)
//         {
//             debug_assert(val == SCIPgetSolVal(scip, nullptr, var));
//             if (SCIPisPositive(scip, val))
//             {
//                 for (PathLength idx = 0; idx < path_length - 1; ++idx)
//                 {
//                     // Get the edge.
//                     const auto i = std::min(path[idx], path[idx + 1]);
//                     const auto j = std::max(path[idx], path[idx + 1]);
//                     debug_assert(i != j);
//
//                     // Store the value of the edge variable.
//                     edge_vals[{i, j}] += val;
//                 }
//             }
//         }
//     }
//
//     // Store the edges in the CVRPSEP format.
//     auto& edge_tail = cvrpsep.edge_tail;
//     auto& edge_head = cvrpsep.edge_head;
//     auto& edge_val = cvrpsep.edge_val;
//     edge_tail.clear();
//     edge_head.clear();
//     edge_val.clear();
//     for (const auto [edge, val] : edge_vals)
//     {
//         // debugln("Edge ({},{}) = {}", instance.vertex_name[edge.i], instance.vertex_name[edge.j], val);
//         debug_assert(SCIPisGT(scip, val, 0.0));
//         debug_assert(edge.i < edge.j);
//
//         edge_tail.push_back(edge.i + 1);
//         edge_head.push_back(edge.j + 1);
//         edge_val.push_back(val);
//     }
//
//     // Check.
// #ifdef DEBUG
//     {
//         Vector<Float> incidence(nb_customers + 2);
//         for (size_t idx = 0; idx < edge_tail.size(); ++idx)
//         {
//             incidence[edge_tail[idx]] += edge_val[idx];
//             incidence[edge_head[idx]] += edge_val[idx];
//         }
//         for (Vertex i = 1; i <= nb_customers; ++i)
//         {
//             release_assert(SCIPisEQ(scip, incidence[i], 2.0));
//         }
//     }
// #endif
//
//     // Set parameters.
//     constexpr int MaxNoOfCuts = 10;
//     constexpr double EpsForIntegrality = 0.0001;
//
//     // Call separation. Pass the previously found cuts in cmp_old.
//     auto cmp = cvrpsep.cmp;
//     auto cmp_old = cvrpsep.cmp_old;
//     char IntegerAndFeasible = 0;
//     double MaxViolation = 0;
//     CAPSEP_SeparateCapCuts(nb_customers,
//                            cvrpsep.demand.data() - 1,
//                            instance.vehicle_load_capacity,
//                            edge_tail.size(),
//                            edge_tail.data() - 1,
//                            edge_head.data() - 1,
//                            edge_val.data() - 1,
//                            cmp_old,
//                            MaxNoOfCuts,
//                            EpsForIntegrality,
//                            &IntegerAndFeasible,
//                            &MaxViolation,
//                            cmp);
//
//     // Add the cuts.
//     if (!IntegerAndFeasible && cmp->Size > 0)
//     {
//         // Process each cut.
//         Vector<Vertex> vertices_in_cut;
//         for (int cut_idx = 0; cut_idx < cmp->Size; ++cut_idx)
//         {
//             // Get the set of vertices in the cut.
//             vertices_in_cut.clear();
// #ifdef DEBUG
//             Load total_load = 0;
// #endif
//             for (int idx = 1; idx <= cmp->CPL[cut_idx]->IntListSize; ++idx)
//             {
//                 const auto vertex = cmp->CPL[cut_idx]->IntList[idx];
//                 debug_assert(1 <= vertex && vertex <= nb_customers);
//                 const auto i = vertex - 1;
//                 vertices_in_cut.push_back(i);
// #ifdef DEBUG
//                 total_load += instance.vertex_load[i];
// #endif
//             }
//
//             // Get the RHS in the form of x(S:S) <= |S| - k(S).
//             const auto rhs = cmp->CPL[cut_idx]->RHS;
//             debug_assert(total_load > instance.vehicle_load_capacity * (vertices_in_cut.size() - rhs - 1));
//
//             // Print.
// #ifdef PRINT_DEBUG
//             println("    Vertices in cut with total load {}, RHS {}, k = {}:", total_load, rhs, vertices_in_cut.size() - rhs);
//             for (const auto i : vertices_in_cut)
//             {
//                 println("        {} with load {}", instance.vertex_name[i], instance.vertex_load[i]);
//             }
// #endif
//
//             // Determine if the cut is violated on the original edges.
//             Float lhs = 0;
//             for (size_t idx1 = 0; idx1 < vertices_in_cut.size() - 1; ++idx1)
//                 for (size_t idx2 = idx1 + 1; idx2 < vertices_in_cut.size(); ++idx2)
//                 {
//                     const auto i = vertices_in_cut[idx1];
//                     const auto j = vertices_in_cut[idx2];
//
//                     if (auto it = edge_vals.find({i, j}); it != edge_vals.end())
//                     {
//                         lhs += it->second;
//                     }
//                     if (auto it = edge_vals.find({j, i}); it != edge_vals.end())
//                     {
//                         lhs += it->second;
//                     }
//                 }
//
//             // Create the cut if violated on the original edges.
//             if (SCIPisGT(scip, lhs, rhs))
//             {
//                 // Print.
//                 debugln("    Creating rounded capacity cut on vertices {} with LHS {} and RHS {} in "
//                         "branch-and-bound node {}",
//                         format_path_with_names(vertices_in_cut.data(), vertices_in_cut.size(), instance.vertex_name),
//                         lhs,
//                         rhs,
//                         SCIPnodeGetNumber(SCIPgetCurrentNode(scip)));
//
//                 // Create cut.
//                 SCIP_CALL(rounded_capacity_create_cut(scip, problem, instance, sepa, vertices_in_cut, rhs, result));
//             }
//             else
//             {
//                 debugln("    Cut not violated on the original edges");
//             }
//         }
//
//         // Move the new cuts to the list of old cuts.
//         for (int i = 0; i < cmp->Size; i++)
//         {
//             CMGR_MoveCnstr(cmp, cmp_old, i, 0);
//         }
//         cmp->Size = 0;
//     }
//
//     // Done.
//     debugln("    Separator completed in {:.4f} seconds", get_clock(scip) - start_time);
//     return SCIP_OKAY;
// }
//
// // Copy method for separator
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// static
// SCIP_DECL_SEPACOPY(sepaCopyRoundedCapacity)
// {
//     // Check.
//     debug_assert(scip);
//     debug_assert(sepa);
//     debug_assert(strcmp(SCIPsepaGetName(sepa), SEPA_NAME) == 0);
//
//     // Include separator.
//     SCIP_CALL(SCIPincludeSepaRoundedCapacity(scip));
//
//     // Done.
//     return SCIP_OKAY;
// }
// #pragma GCC diagnostic pop
//
// // Separation method for LP solutions
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// static
// SCIP_DECL_SEPAEXECLP(sepaExeclpRoundedCapacity)
// {
//     // Check.
//     debug_assert(scip);
//     debug_assert(sepa);
//     debug_assert(strcmp(SCIPsepaGetName(sepa), SEPA_NAME) == 0);
//     debug_assert(result);
//
//     // Start.
//     *result = SCIP_DIDNOTFIND;
//
//     // Start separator.
// #ifdef ROUNDED_CAPACITY_CUTS_MAX_DEPTH
//     if (SCIPgetDepth(scip) <= ROUNDED_CAPACITY_CUTS_MAX_DEPTH)
// #endif
//     {
//         SCIP_CALL(rounded_capacity_separate(scip, sepa, result));
//     }
//
//     // Done.
//     return SCIP_OKAY;
// }
// #pragma GCC diagnostic pop
//
// // Create separator for rounded capacity cuts and include it in SCIP
// SCIP_RETCODE SCIPincludeSepaRoundedCapacity(
//     SCIP* scip    // SCIP
// )
// {
//     // Check.
//     debug_assert(scip);
//
//     // Include separator.
//     SCIP_Sepa* sepa = nullptr;
//     SCIP_CALL(SCIPincludeSepaBasic(scip,
//                                    &sepa,
//                                    SEPA_NAME,
//                                    SEPA_DESC,
//                                    SEPA_PRIORITY,
//                                    SEPA_FREQ,
//                                    SEPA_MAXBOUNDDIST,
//                                    SEPA_USESSUBSCIP,
//                                    SEPA_DELAY,
//                                    sepaExeclpRoundedCapacity,
//                                    nullptr,
//                                    nullptr));
//     debug_assert(sepa);
//
//     // Set callbacks.
//     SCIP_CALL(SCIPsetSepaCopy(scip, sepa, sepaCopyRoundedCapacity));
//
//     // Done.
//     return SCIP_OKAY;
// }
//
// #endif
