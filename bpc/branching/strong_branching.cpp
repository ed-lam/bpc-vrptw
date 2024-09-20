// // #define PRINT_DEBUG
//
// #include "branching/strong_branching.h"
// #include "inequalities/robust_cut.h"
// #include "inequalities/subset_row_cut.h"
// #include "problem/instance.h"
// #include "problem/problem.h"
// #include <scip/scipdefplugins.h>
//
// // Compute pseudocosts by strong branching
// SCIP_RETCODE strong_branching_for_pseudocosts(
//     SCIP* scip,                                          // SCIP
//     HashMap<Edge, PseudocostsData>& pseudocosts,         // Pseudocosts of the edges
//     const HashMap<Edge, SCIP_Real>& fractional_edges,    // Edges with fractional value
//     SCIP_Real current_lb                                 // LP objective value of current node
// )
// {
//     // Check.
//     release_assert(!fractional_edges.empty(), "No fractional edges for branching");
//
//     // Get instance.
//     auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
//     const auto& instance = *problem.instance;
//     const auto nb_customers = instance.nb_customers();
//
//     // Initialize sub-SCIP.
//     SCIP* subscip;
//     SCIP_CALL(SCIPcreate(&subscip));
//     SCIP_CALL(SCIPincludeDefaultPlugins(subscip));
//
//     // Create problem in sub-SCIP.
//     SCIP_CALL(SCIPcreateProbBasic(subscip, "strong_branching_subscip"));
//
//     // Set optimization direction.
//     SCIP_CALL(SCIPsetObjsense(subscip, SCIP_OBJSENSE_MINIMIZE));
//
//     // Tell SCIP that the objective value will be always integral.
//     SCIP_CALL(SCIPsetObjIntegral(subscip));
//
//     // Do not abort subproblem on CTRL-C.
//     SCIP_CALL(SCIPsetBoolParam(subscip, "misc/catchctrlc", FALSE));
//
//     // Disable output to console.
//     SCIP_CALL(SCIPsetIntParam(subscip, "display/verblevel", 0));
//
//     // Add variables.
//     Vector<PathVariable> vars(problem.vars);
//     for (auto& [var, _, path, path_length] : vars)
//     {
//         SCIP_CALL(SCIPcreateVar(subscip,
//                                 &var,
//                                 SCIPvarGetName(var),
//                                 0.0,
//                                 1.0,
//                                 SCIPvarGetObj(var),
//                                 SCIP_VARTYPE_CONTINUOUS,
//                                 TRUE,
//                                 FALSE,
//                                 nullptr,
//                                 nullptr,
//                                 nullptr,
//                                 nullptr,
//                                 nullptr));
//         SCIP_CALL(SCIPaddVar(subscip, var));
//     }
//
//     // Create the customer cover constraints.
//     Vector<SCIP_Cons*> conss;
//     for (Vertex i = 0; i < nb_customers; ++i)
//     {
//         // Calculate the coefficients of the variables.
//         Vector<SCIP_Var*> cons_vars;
//         Vector<SCIP_Real> cons_coeffs;
//         for (const auto& [var, _, path, path_length] : vars)
//         {
//             SCIP_Real coeff = 0.0;
//             for (PathLength idx = 1; idx < path_length - 1; ++idx)
//             {
//                 const auto vertex = path[idx];
//                 coeff += (vertex == i);
//             }
//             if (coeff)
//             {
//                 cons_vars.push_back(var);
//                 cons_coeffs.push_back(coeff);
//             }
//         }
//
//         // Create the constraint.
// #ifdef DEBUG
//         const auto name = fmt::format("customer_cover({})", instance.vertex_name[i]).substr(0, 200);
//         const auto name_c_str = name.c_str();
// #else
//         constexpr auto name_c_str = "";
// #endif
//         auto& cons = conss.emplace_back();
//         SCIP_CALL(SCIPcreateConsLinear(subscip,
//                                        &cons,
//                                        name_c_str,
//                                        cons_vars.size(),
//                                        cons_vars.data(),
//                                        cons_coeffs.data(),
//                                        1.0,
//                                        1.0,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE));
//         debug_assert(cons);
//
//         // Add the constraint to the problem.
//         SCIP_CALL(SCIPaddCons(subscip, cons));
//     }
//
//     // Create the robust cuts.
//     const auto& robust_cuts = problem.robust_cuts;
//     for (const auto& cut : robust_cuts)
//     {
//         // Calculate the coefficients.
//         Vector<SCIP_Var*> cons_vars;
//         Vector<SCIP_Real> cons_coeffs;
//         for (const auto& [var, _, path, path_length] : vars)
//         {
//             // Calculate the coefficient.
//             SCIP_Real coeff = 0.0;
//             for (PathLength idx = 0; idx < path_length - 1; ++idx)
//             {
//                 const Edge edge{path[idx], path[idx + 1]};
//                 coeff += cut.coeff(edge);
//             }
//
//             // Add variable to the cut.
//             if (coeff)
//             {
//                 cons_vars.push_back(var);
//                 cons_coeffs.push_back(coeff);
//             }
//         }
//
//         // Create the constraint.
//         auto& cons = conss.emplace_back();
//         SCIP_CALL(SCIPcreateConsLinear(subscip,
//                                        &cons,
// #ifdef DEBUG
//                                        cut.name().substr(0, 200).c_str(),
// #else
//                                        "",
// #endif
//                                        cons_vars.size(),
//                                        cons_vars.data(),
//                                        cons_coeffs.data(),
//                                        cut.rhs(),
//                                        SCIPinfinity(subscip),
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE));
//         debug_assert(cons);
//
//         // Add the constraint to the problem.
//         SCIP_CALL(SCIPaddCons(subscip, cons));
//     }
//
//     // Create the subset row cuts.
// #ifdef USE_SUBSET_ROW_CUTS
//     const auto& subset_row_cuts = problem.subset_row_cuts;
//     for (const auto& cut : subset_row_cuts)
//     {
//         // Calculate the coefficients.
//         Vector<SCIP_Var*> cons_vars;
//         Vector<SCIP_Real> cons_coeffs;
//         for (const auto& [var, _, path, path_length] : vars)
//         {
//             // Calculate the coefficient.
//             SCIP_Real coeff = 0;
//             for (PathLength idx = 1; idx < path_length - 1; ++idx)
//             {
//                 const auto vertex = path[idx];
//                 coeff += (vertex == cut.i()) || (vertex == cut.j()) || (vertex == cut.k());
//             }
//             coeff = std::floor(coeff / 2);
//
//             // Add variable to the cut.
//             if (coeff)
//             {
//                 cons_vars.push_back(var);
//                 cons_coeffs.push_back(coeff);
//             }
//         }
//
//         // Create the constraint.
//         auto& cons = conss.emplace_back();
//         SCIP_CALL(SCIPcreateConsLinear(subscip,
//                                        &cons,
// #ifdef DEBUG
//                                        cut.name().substr(0, 200).c_str(),
// #else
//                                        "",
// #endif
//                                        cons_vars.size(),
//                                        cons_vars.data(),
//                                        cons_coeffs.data(),
//                                        -SCIPinfinity(subscip),
//                                        1.0,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        TRUE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE,
//                                        FALSE));
//         debug_assert(cons);
//
//         // Add the constraint to the problem.
//         SCIP_CALL(SCIPaddCons(subscip, cons));
//     }
// #endif
//
//     // Select a random subset of edges to perform strong branching.
//     Vector<Pair<Edge, SCIP_Real>> candidates(fractional_edges.size());
//     {
//         Count idx = 0;
//         for (const auto& [edge, edge_val] : fractional_edges)
//         {
//             candidates[idx] = {edge, edge_val};
//             ++idx;
//         }
//         debug_assert(idx == static_cast<Count>(fractional_edges.size()));
//     }
//     if (candidates.size() > STRONG_BRANCHING_NUM_EDGES_FOR_STRONG_BRANCHING)
//     {
//         auto& rng = problem.rng;
//         std::shuffle(candidates.begin(), candidates.end(), rng);
//
//         candidates.resize(STRONG_BRANCHING_NUM_EDGES_FOR_STRONG_BRANCHING);
//     }
//
//     // Perform strong branching.
//     Vector<SCIP_Var*> disabled_vars;
//     for (const auto& [edge, edge_val] : candidates)
//     {
//         // Get the pseudocosts of the edge.
//         auto& edge_pseudocosts = pseudocosts.at(edge);
//
//         // Check.
//         debug_assert(edge_pseudocosts.down_ntimes == 0);
//         debug_assert(edge_pseudocosts.up_ntimes == 0);
//
//         // Branch down.
//         {
//             // Print.
//             debugln("    Performing strong branching down on edge ({},{}) with value {:.4f}",
//                     instance.vertex_name[edge.i],
//                     instance.vertex_name[edge.j],
//                     edge_val);
//
//             // Disable variables.
//             disabled_vars.clear();
//             for (const auto& [var, _, path, path_length] : vars)
//             {
//                 // Determine if the variable should be disabled.
//                 for (PathLength idx = 0; idx < path_length - 1; ++idx)
//                 {
//                     const auto i = path[idx];
//                     const auto j = path[idx + 1];
//                     if (i == edge.i && j == edge.j)
//                     {
//                         // Disable the variable.
//                         SCIP_CALL(SCIPchgVarUb(subscip, var, 0.0));
//                         disabled_vars.push_back(var);
//                         debugln("        Disabling {}", SCIPvarGetName(var));
//
//                         // Advance to the next variable.
//                         break;
//                     }
//                 }
//             }
//
//             // Set time and memory limit.
//             {
//                 SCIP_Real time_limit;
//                 SCIP_Real memory_limit;
//                 SCIP_CALL(SCIPgetRealParam(scip, "limits/time", &time_limit));
//                 if (!SCIPisInfinity(scip, time_limit))
//                 {
//                     time_limit -= SCIPgetSolvingTime(scip);
//                 }
//                 SCIP_CALL(SCIPgetRealParam(scip, "limits/memory", &memory_limit));
//                 if (!SCIPisInfinity(scip, memory_limit))
//                 {
//                     memory_limit -= SCIPgetMemUsed(scip) / 1048576.0;
//                 }
//                 if (time_limit < 0 || memory_limit < 0)
//                 {
//                     goto EXIT;
//                 }
//                 SCIP_CALL(SCIPsetRealParam(subscip, "limits/time", time_limit));
//                 SCIP_CALL(SCIPsetRealParam(subscip, "limits/memory", memory_limit));
//             }
//
//             // Write model to file.
//             // SCIP_CALL(SCIPwriteOrigProblem(subscip, "stronger_branching_down.lp", "lp", false));
//             // write_master(scip);
//
//             // Solve.
//             SCIP_CALL(SCIPsolve(subscip));
//
//             // Get the objective value.
//             const auto child_lb = SCIPgetBestSol(subscip) ?
//                                   SCIPgetSolOrigObj(subscip, SCIPgetBestSol(subscip)) :
//                                   current_lb + STRONG_BRANCHING_INFEASIBLE_LB_VALUE;
//             debug_assert(SCIPisGE(subscip, child_lb, current_lb));
//
//             // Compute the pseudocost.
//             debug_assert(std::isnan(edge_pseudocosts.down_pcost_sum));
//             const auto pcost = (child_lb - current_lb) / edge_val;
//             edge_pseudocosts.down_pcost_sum = pcost;
//             edge_pseudocosts.down_ntimes = 1;
//             debugln("    Down branch dual bound {:.2f}, pseudocost {:.2f}", child_lb, pcost);
//             debugln("");
//
//             // Undo solve.
//             SCIP_CALL(SCIPfreeTransform(subscip));
//
//             // Enable variables.
//             for (auto var : disabled_vars)
//             {
//                 SCIP_CALL(SCIPchgVarUb(subscip, var, 1.0));
//             }
//         }
//
//         // Branch up.
//         {
//             // Print.
//             debugln("    Performing strong branching up on edge ({},{}) with value {:.4f}",
//                     instance.vertex_name[edge.i],
//                     instance.vertex_name[edge.j],
//                     edge_val);
//
//             // Disable variables.
//             disabled_vars.clear();
//             for (const auto& [var, _, path, path_length] : vars)
//             {
//                 // Determine if the variable should be disabled.
//                 auto disable_path = false;
//                 if (edge.i == path[0])
//                 {
//                     // Disable if anywhere goes to j that is not i.
//                     for (PathLength idx = 0; idx < path_length - 1; ++idx)
//                     {
//                         const auto i = path[idx];
//                         const auto j = path[idx + 1];
//                         if (i != edge.i && j == edge.j)
//                         {
//                             disable_path = true;
//                             break;
//                         }
//                     }
//                 }
//                 else if (edge.j == path[path_length - 1])
//                 {
//                     // Disable if i goes to anywhere that is not j.
//                     for (PathLength idx = 0; idx < path_length - 1; ++idx)
//                     {
//                         const auto i = path[idx];
//                         const auto j = path[idx + 1];
//                         if (i == edge.i && j != edge.j)
//                         {
//                             disable_path = true;
//                             break;
//                         }
//                     }
//                 }
//                 else
//                 {
//                     for (PathLength idx = 0; idx < path_length - 1; ++idx)
//                     {
//                         const auto i = path[idx];
//                         const auto j = path[idx + 1];
//                         if ((i == edge.i) ^ (j == edge.j))
//                         {
//                             disable_path = true;
//                             break;
//                         }
//                     }
//                 }
//
//                 // Disable the variable.
//                 if (disable_path)
//                 {
//                     SCIP_CALL(SCIPchgVarUb(subscip, var, 0.0));
//                     disabled_vars.push_back(var);
//                     debugln("        Disabling {}", SCIPvarGetName(var));
//                 }
//             }
//
//             // Set time and memory limit.
//             {
//                 SCIP_Real time_limit;
//                 SCIP_Real memory_limit;
//                 SCIP_CALL(SCIPgetRealParam(scip, "limits/time", &time_limit));
//                 if (!SCIPisInfinity(scip, time_limit))
//                 {
//                     time_limit -= SCIPgetSolvingTime(scip);
//                 }
//                 SCIP_CALL(SCIPgetRealParam(scip, "limits/memory", &memory_limit));
//                 if (!SCIPisInfinity(scip, memory_limit))
//                 {
//                     memory_limit -= SCIPgetMemUsed(scip) / 1048576.0;
//                 }
//                 if (time_limit < 0 || memory_limit < 0)
//                 {
//                     goto EXIT;
//                 }
//                 SCIP_CALL(SCIPsetRealParam(subscip, "limits/time", time_limit));
//                 SCIP_CALL(SCIPsetRealParam(subscip, "limits/memory", memory_limit));
//             }
//
//             // Write model to file.
//             // SCIP_CALL(SCIPwriteOrigProblem(subscip, "stronger_branching_up.lp", "lp", false));
//             // write_master(scip);
//
//             // Solve.
//             SCIP_CALL(SCIPsolve(subscip));
//
//             // Get the objective value.
//             const auto child_lb = SCIPgetBestSol(subscip) ?
//                                   SCIPgetSolOrigObj(subscip, SCIPgetBestSol(subscip)) :
//                                   current_lb + STRONG_BRANCHING_INFEASIBLE_LB_VALUE;
//             debug_assert(SCIPisGE(subscip, child_lb, current_lb));
//
//             // Compute the pseudocost.
//             debug_assert(std::isnan(edge_pseudocosts.up_pcost_sum));
//             const auto pcost = (child_lb - current_lb) / (1.0 - edge_val);
//             edge_pseudocosts.up_pcost_sum = pcost;
//             edge_pseudocosts.up_ntimes = 1;
//             debugln("    Up branch dual bound {:.2f}, pseudocost {:.2f}", child_lb, pcost);
//             debugln("");
//
//             // Undo solve.
//             SCIP_CALL(SCIPfreeTransform(subscip));
//
//             // Enable variables.
//             for (auto var : disabled_vars)
//             {
//                 SCIP_CALL(SCIPchgVarUb(subscip, var, 1.0));
//             }
//         }
//     }
//
//     // Free constraints.
//     EXIT:
//     for (auto& cons : conss)
//     {
//         SCIPreleaseCons(subscip, &cons);
//     }
//
//     // Free variables.
//     for (auto& [var, _, path, path_length] : vars)
//     {
//         SCIPreleaseVar(subscip, &var);
//     }
//
//     // Free memory.
//     SCIP_CALL(SCIPfree(&subscip));
//
//     // For every edge that wasn't evaluated, set its pseudocosts to the average.
//     {
//         // Compute the average pseudocost for both directions.
//         SCIP_Real avg_down_pcost = 0.0;
//         SCIP_Real avg_up_pcost = 0.0;
//         SCIP_Real down_ntimes = 0;
//         SCIP_Real up_ntimes = 0;
//         for (const auto& [edge, edge_pseudocosts] : pseudocosts)
//         {
//             if (edge_pseudocosts.down_ntimes > 0)
//             {
//                 avg_down_pcost += edge_pseudocosts.down_pcost_sum;
//                 ++down_ntimes;
//             }
//             if (edge_pseudocosts.up_ntimes > 0)
//             {
//                 avg_up_pcost += edge_pseudocosts.up_pcost_sum;
//                 ++up_ntimes;
//             }
//         }
//         avg_down_pcost /= down_ntimes;
//         avg_up_pcost /= up_ntimes;
//
//         // Set to the average pseudocost.
//         for (auto& [edge, edge_pseudocosts] : pseudocosts)
//         {
//             if (edge_pseudocosts.down_ntimes == 0)
//             {
//                 edge_pseudocosts.down_pcost_sum = avg_down_pcost;
//                 edge_pseudocosts.down_ntimes = 1;
//             }
//             if (edge_pseudocosts.up_ntimes == 0)
//             {
//                 edge_pseudocosts.up_pcost_sum = avg_up_pcost;
//                 edge_pseudocosts.up_ntimes = 1;
//             }
//         }
//     }
//
//     // Print.
// #ifdef PRINT_DEBUG
//     debugln("    Pseudocosts of fractional edges:");
//     for (const auto& [edge, _] : fractional_edges)
//     {
//         debugln("        Edge ({},{}): down pseudocost {:.2f}, up pseudocost {:.2f}",
//                 instance.vertex_name[edge.i],
//                 instance.vertex_name[edge.j],
//                 pseudocosts.at(edge).down_pcost_sum,
//                 pseudocosts.at(edge).up_pcost_sum);
//     }
// //    debugln("    Pseudocosts of all edges:");
// //    for (const auto& [edge, edge_pseudocosts] : pseudocosts)
// //    {
// //        debugln("        Edge ({},{}): down pseudocost {:.2f}, up pseudocost {:.2f}",
// //                instance.vertex_name[edge.i],
// //                instance.vertex_name[edge.j],
// //                edge_pseudocosts.down_pcost_sum,
// //                edge_pseudocosts.up_pcost_sum);
// //    }
// #endif
//
//     // Done.
//     return SCIP_OKAY;
// }
