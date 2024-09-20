// #ifdef USE_TWO_PATH_CUTS
//
// // #define PRINT_DEBUG
//
// #include "inequalities/separator_two_path.h"
// #include "inequalities/robust_cut.h"
// #include "inequalities/two_path_labeling_algorithm.h"
// #include "output/formatting.h"
// #include "problem/clock.h"
// #include "problem/instance.h"
// #include "problem/problem.h"
// #include "types/array.h"
// #include "types/edge.h"
// #include <random>
//
// #define SEPA_NAME         "two_path"
// #define SEPA_DESC         "Separator for two-path cuts"
// #define SEPA_PRIORITY     2                                // priority of the constraint handler for separation
// #define SEPA_FREQ         1                                // frequency for separating cuts; zero means to separate only in the root node
// #define SEPA_MAXBOUNDDIST 1.0
// #define SEPA_USESSUBSCIP  FALSE                            // does the separator use a secondary SCIP instance?
// #define SEPA_DELAY        TRUE                             // should separation method be delayed, if other separators found cuts?
//
// SCIP_RETCODE two_path_create_cut(
//     SCIP* scip,                           // SCIP
//     Problem& problem,                     // Problem
//     const Instance& instance,             // Instance
//     SCIP_Sepa* sepa,                      // Separator
//     const Vector<Bool>& vertex_in_cut,    // Indicates if a vertex is in the cut
//     SCIP_Result* result                   // Output result
// )
// {
//     // Get instance.
//     const auto nb_vertices = instance.nb_vertices();
//
//     // Create constraint name.
// #ifdef DEBUG
//     Vector<Vertex> vertices_in_cut;
//     for (Vertex i = 0; i < static_cast<Vertex>(vertex_in_cut.size()); ++i)
//         if (vertex_in_cut[i])
//         {
//             vertices_in_cut.push_back(i);
//         }
//     auto name = fmt::format("two_path({})",
//                             format_path_with_names(vertices_in_cut.data(),
//                                                      vertices_in_cut.size(),
//                                                      instance.vertex_name,
//                                                      ","));
// #endif
//
//     // Count the number of edges in the cut.
//     Count count = 0;
//     for (Vertex i = 0; i < nb_vertices; ++i)
//         for (Vertex j = 0; j < nb_vertices; ++j)
//             if (!vertex_in_cut[i] && vertex_in_cut[j] && instance.is_valid(i, j))
//             {
//                 ++count;
//             }
//
//     // Create constraint.
//     RobustCut cut(scip,
//                   count,
//                   2
// #ifdef DEBUG
//                 , std::move(name)
// #endif
//     );
//     Count idx = 0;
//     for (Vertex i = 0; i < nb_vertices; ++i)
//         for (Vertex j = 0; j < nb_vertices; ++j)
//             if (!vertex_in_cut[i] && vertex_in_cut[j] && instance.is_valid(i, j))
//             {
//                 cut.edges(idx) = EdgeVal{i, j, 1};
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
// bool load_infeasible(
//     const Instance& instance,                           // Instance
//     const TwoPathCutsVerticesSubset& vertices_in_cut    // Vertices in the cut
// )
// {
//     // Print.
//     debugln("            Checking load on vertices {}",
//             format_path_with_names(vertices_in_cut.data(), vertices_in_cut.size(), instance.vertex_name));
//
//     // Get vertices data.
//     const auto& vertex_load = instance.vertex_load;
//
//     // Get vehicles data.
//     const auto vehicle_load_capacity = instance.vehicle_load_capacity;
//
//     // Sum.
//     Load load = 0;
//     for (Count idx = 0; idx < vertices_in_cut.size(); ++idx)
//     {
//         const auto i = vertices_in_cut[idx];
//         load += vertex_load[i];
//     }
//
//     // Return failure.
// #ifdef PRINT_DEBUG
//     if (load > vehicle_load_capacity)
//     {
//         debugln("                Load infeasible");
//     }
//     else
//     {
//         debugln("                Load feasible");
//     }
// #endif
//     return load > vehicle_load_capacity;
// }
//
// inline bool tsptw_infeasible(
//     TwoPathLabelingAlgorithm& two_path_labeling_algorithm,    // TSPTW solver
//     const TwoPathCutsVerticesSubset& vertices_in_cut          // Vertices in the cut
// )
// {
//     return !two_path_labeling_algorithm.solve(vertices_in_cut);
// }
//
// bool one_path_infeasible(
//     const Instance& instance,                                      // Instance
//     TwoPathLabelingAlgorithm& two_path_labeling_algorithm,         // TSPTW solver
//     HashMap<TwoPathCutsVerticesSubset, Bool>& one_path_checked,    // Sets of vertices previously checked for one-path infeasibility
//     const TwoPathCutsVerticesSubset& vertices_in_cut               // Vertices in the cut
// )
// {
//     // Determine whether this set of vertices has been checked previously.
//     auto sorted_vertices_in_cut = vertices_in_cut;
//     std::sort(sorted_vertices_in_cut.begin(), sorted_vertices_in_cut.end());
//     auto it = one_path_checked.find(sorted_vertices_in_cut);
//     bool infeasible;
//     if (it == one_path_checked.end())
//     {
//         infeasible = load_infeasible(instance, sorted_vertices_in_cut) ||
//                      tsptw_infeasible(two_path_labeling_algorithm, sorted_vertices_in_cut);
//         one_path_checked[sorted_vertices_in_cut] = infeasible;
//     }
//     else
//     {
//         infeasible = it->second;
//         debugln("            Vertices previously found to be {}", infeasible ? "infeasible" : "feasible");
//     }
//     return infeasible;
// }
//
// void recurse_exhaustive(
//     SCIP* scip,                                                    // SCIP
//     Problem& problem,                                              // Problem
//     HashMap<TwoPathCutsVerticesSubset, Bool>& one_path_checked,    // Sets of vertices previously checked for one-path infeasibility
//     TwoPathLabelingAlgorithm& two_path_labeling_algorithm,         // TSPTW solver
//     const Instance& instance,                                      // Instance
//     SCIP_Sepa* sepa,                                               // Separator
//     const HashMap<Edge, Float>& edge_vals,                         // Value of the implicit edge variables
//     Count& iteration_count,                                        // Iteration count
//     const Int32 position,                                          // Position in the list of vertices to include
//     const Vertex* const customer_vertices_to_check,                // Vertices to include in the set
//     const Count nb_customer_vertices_to_check,                     // Number of vertices in the large set
//     TwoPathCutsVerticesSubset& vertices_in_cut,                    // Vertices in the cut
//     Vector<Bool>& vertex_in_cut,                                   // Indicates if a vertex is in the cut
//     SCIP_Result* result                                            // Output result
// )
// {
//     // Stop if reached the end of the set of vertices to check or reached the iteration limit.
//     iteration_count++;
//     if (position == nb_customer_vertices_to_check || iteration_count >= TWO_PATH_ITERATION_LIMIT)
//     {
//         return;
//     }
//
//     // Recurse on the next position without the customer.
//     recurse_exhaustive(scip,
//                        problem,
//                        one_path_checked,
//                        two_path_labeling_algorithm,
//                        instance,
//                        sepa,
//                        edge_vals,
//                        iteration_count,
//                        position + 1,
//                        customer_vertices_to_check,
//                        nb_customer_vertices_to_check,
//                        vertices_in_cut,
//                        vertex_in_cut,
//                        result);
//
//     // Stop if the subset has the maximum size or reached the iteration limit.
//     if (vertices_in_cut.size() == TWO_PATH_CUTS_MAX_SUBSET_SIZE || iteration_count >= TWO_PATH_ITERATION_LIMIT)
//     {
//         return;
//     }
//
//     // Add the customer to the set.
//     const auto vertex = customer_vertices_to_check[position];
//     vertices_in_cut.push_back(vertex);
//     debug_assert(!vertex_in_cut[vertex]);
//     vertex_in_cut[vertex] = true;
//
//     // Create a cut if it is violated.
//     if (vertices_in_cut.size() >= TWO_PATH_CUTS_MIN_SUBSET_SIZE)
//     {
//         // Calculate the LHS.
//         Float lhs = 0;
//         for (const auto [e, val] : edge_vals)
//             if (!vertex_in_cut[e.i] && vertex_in_cut[e.j])
//             {
//                 lhs += val;
//             }
//
//         // Create the cut if it is violated.
//         debugln("        Checking vertices {} with LHS {}",
//                 format_path_with_names(vertices_in_cut.data(), vertices_in_cut.size(), instance.vertex_name),
//                 lhs);
//         if (SCIPisLT(scip, lhs, 2.0) &&
//             one_path_infeasible(instance, two_path_labeling_algorithm, one_path_checked, vertices_in_cut))
//         {
//             // Print.
//             debugln("            Creating two-path cut on vertices {} with LHS {} in branch-and-bound node {}",
//                     format_path_with_names(vertices_in_cut.data(),
//                                              vertices_in_cut.size(),
//                                              instance.vertex_name),
//                     lhs,
//                     SCIPnodeGetNumber(SCIPgetCurrentNode(scip)));
//
//             // Create cut.
//             scip_assert(two_path_create_cut(scip, problem, instance, sepa, vertex_in_cut, result));
//
//             // Stop checking this connected component.
//             iteration_count = TWO_PATH_ITERATION_LIMIT;
//             return;
//         }
//     }
//
//     // Recurse on the next position after adding the customer.
//     recurse_exhaustive(scip,
//                        problem,
//                        one_path_checked,
//                        two_path_labeling_algorithm,
//                        instance,
//                        sepa,
//                        edge_vals,
//                        iteration_count,
//                        position + 1,
//                        customer_vertices_to_check,
//                        nb_customer_vertices_to_check,
//                        vertices_in_cut,
//                        vertex_in_cut,
//                        result);
//
//     // Stop if reached the iteration limit.
//     if (iteration_count >= TWO_PATH_ITERATION_LIMIT)
//     {
//         return;
//     }
//
//     // Remove the added vertex.
//     debug_assert(vertices_in_cut.back() == vertex);
//     vertices_in_cut.pop_back();
//     debug_assert(vertex_in_cut[vertex]);
//     vertex_in_cut[vertex] = false;
// }
//
// inline Vertex find_root(const Vector<Vertex>& parent, Vertex i)
// {
//     while (parent[i] != i)
//     {
//         i = parent[i];
//         debug_assert(i != -1);
//     }
//     return i;
// }
//
// // Separator
// static
// SCIP_RETCODE two_path_separate(
//     SCIP* scip,            // SCIP
//     SCIP_Sepa* sepa,       // Separator
//     SCIP_RESULT* result    // Pointer to store the result of the separation call
// )
// {
//     // Print.
//     debugln("Starting separator for two-path cuts at node {}, depth {}, obj {}:",
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
//     const auto nb_vertices = instance.nb_vertices();
//     const auto depot = instance.depot();
//     auto& rng = problem.rng;
//     auto& one_path_checked = *problem.one_path_checked;
//     auto& two_path_labeling_algorithm = *problem.two_path_labeling_algorithm;
//
//     // Get the value of the edge variables.
//     Vector<Vertex> union_find_parent(nb_customers, -1);
//     HashMap<Edge, Float> edge_vals;
//     {
//         const auto& vars = problem.vars;
//         for (const auto& [var, val, path, path_length] : vars)
//         {
//             // Store the vertices and edges of the path.
//             debug_assert(val == SCIPgetSolVal(scip, nullptr, var));
//             if (!SCIPisIntegral(scip, val))
//             {
//                 // Process each edge.
//                 PathLength idx = 0;
//                 for (; idx < path_length - 2; ++idx)
//                 {
//                     const auto i = path[idx];
//                     const auto j = path[idx + 1];
//                     debug_assert(i != j);
//
//                     edge_vals[{i, j}] += val;
//
//                     debug_assert(j != depot);
//                     if (union_find_parent[j] == -1)
//                     {
//                         union_find_parent[j] = j;
//                     }
//                     if (i != depot && union_find_parent[i] != -1)
//                     {
//                         const auto i_root = find_root(union_find_parent, i);
//                         const auto j_root = find_root(union_find_parent, j);
//                         if (i_root != j_root)
//                         {
//                             union_find_parent[j_root] = i_root;
//                         }
//                     }
//                 }
//                 {
//                     const auto i = path[idx];
//                     const auto j = path[idx + 1];
//                     debug_assert(i != j);
//
//                     edge_vals[{i, j}] += val;
//                 }
//             }
//         }
//     }
//
//     // Print edge values.
// #ifdef PRINT_DEBUG
//     for (const auto [edge, val] : edge_vals)
//     {
//         debugln("    ({},{}): {}", instance.vertex_name[edge.i], instance.vertex_name[edge.j], val);
//     }
// #endif
//
//     // Build the connected components.
//     debugln("    Found connected components:");
//     Vector<Vector<Vertex>> connected_components;
//     for (Vertex i = 0; i < nb_customers; ++i)
//         if (union_find_parent[i] != -1)
//         {
//             union_find_parent[i] = find_root(union_find_parent, i);
//         }
//     {
//         Vertex prev_root_index = -1;
//         while (true)
//         {
//             Vertex root_index = std::numeric_limits<Vertex>::max();
//             for (const auto x : union_find_parent)
//                 if (prev_root_index < x && x < root_index)
//                 {
//                     root_index = x;
//                 }
//             if (root_index == std::numeric_limits<Vertex>::max())
//             {
//                 break;
//             }
//
//             auto& component = connected_components.emplace_back();
//             for (Vertex i = 0; i < nb_customers; ++i)
//                 if (union_find_parent[i] == root_index)
//                 {
//                     component.push_back(i);
//                 }
//
//             prev_root_index = root_index;
//         }
//     }
//     std::shuffle(connected_components.begin(), connected_components.end(), rng);
// #ifdef PRINT_DEBUG
//     for (const auto& component : connected_components)
//     {
//         debugln("        Size {}: {}",
//                 component.size(),
//                 format_path_with_names(component.data(), component.size(), instance.vertex_name));
//     }
// #endif
//
//     // Check each connected component.
//     for (auto& component : connected_components)
//     {
//         // Count the number of customers in the connected component.
//         Count component_nb_customers = 0;
//         while (component_nb_customers < static_cast<Count>(component.size()) &&
//                component[component_nb_customers] < nb_customers)
//         {
//             ++component_nb_customers;
//         }
//
//         // Shuffle the customers.
//         std::shuffle(component.begin(), component.begin() + component_nb_customers, rng);
//
//         // Print.
//         debugln("    Checking connected component of size {}: {}",
//                 component.size(),
//                 format_path_with_names(component.data(), component.size(), instance.vertex_name));
//
//         // Check recursively if including or not including a customer violates a two-path cut.
//         Count iteration_count = 0;
//         for (Count idx = 0; idx < component_nb_customers; idx += TWO_PATH_HEURISTIC_SKIP)
//         {
//             // Calculate the subarray of customers.
//             auto customers_begin = component.data() + idx;
//             auto customers_size = std::min(component_nb_customers - idx, TWO_PATH_CUTS_MAX_SUBSET_SIZE);
//
//             // Select a base customer from which all other customers are connected.
//             TwoPathCutsVerticesSubset vertices_in_cut;
//             Vector<Bool> vertex_in_cut(nb_vertices);
//
//             // Check recursively.
//             recurse_exhaustive(scip,
//                                problem,
//                                one_path_checked,
//                                two_path_labeling_algorithm,
//                                instance,
//                                sepa,
//                                edge_vals,
//                                iteration_count,
//                                0,
//                                customers_begin,
//                                customers_size,
//                                vertices_in_cut,
//                                vertex_in_cut,
//                                result);
//             if (iteration_count >= TWO_PATH_ITERATION_LIMIT)
//             {
//                 break;
//             }
//         }
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
// SCIP_DECL_SEPACOPY(sepaCopyTwoPath)
// {
//     // Check.
//     debug_assert(scip);
//     debug_assert(sepa);
//     debug_assert(strcmp(SCIPsepaGetName(sepa), SEPA_NAME) == 0);
//
//     // Include separator.
//     SCIP_CALL(SCIPincludeSepaTwoPath(scip));
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
// SCIP_DECL_SEPAEXECLP(sepaExeclpTwoPath)
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
// #ifdef TWO_PATH_CUTS_MAX_DEPTH
//     if (SCIPgetDepth(scip) <= TWO_PATH_CUTS_MAX_DEPTH)
// #endif
//     {
//         SCIP_CALL(two_path_separate(scip, sepa, result));
//     }
//
//     // Done.
//     return SCIP_OKAY;
// }
// #pragma GCC diagnostic pop
//
// // Create separator for two-path cuts and include it in SCIP
// SCIP_RETCODE SCIPincludeSepaTwoPath(
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
//                                    sepaExeclpTwoPath,
//                                    nullptr,
//                                    nullptr));
//     debug_assert(sepa);
//
//     // Set callbacks.
//     SCIP_CALL(SCIPsetSepaCopy(scip, sepa, sepaCopyTwoPath));
//
//     // Done.
//     return SCIP_OKAY;
// }
//
// #endif
