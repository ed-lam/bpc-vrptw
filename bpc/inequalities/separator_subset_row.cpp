// #ifdef USE_SUBSET_ROW_CUTS
//
// // #define PRINT_DEBUG
//
// #include "inequalities/separator_subset_row.h"
// #include "inequalities/subset_row_cut.h"
// #include "output/formatting.h"
// #include "problem/clock.h"
// #include "problem/instance.h"
// #include "problem/problem.h"
// #include <algorithm>
//
// #define SEPA_NAME         "subset_row"
// #define SEPA_DESC         "Separator for subset row cuts"
// #define SEPA_PRIORITY     1                                  // priority of the constraint handler for separation
// #define SEPA_FREQ         1                                  // frequency for separating cuts; zero means to separate only in the root node
// #define SEPA_MAXBOUNDDIST 1.0
// #define SEPA_USESSUBSCIP  FALSE                              // does the separator use a secondary SCIP instance?
// #define SEPA_DELAY        TRUE                               // should separation method be delayed, if other separators found cuts?
//
// #define MAX_NB_CUTS (30)
// #define MAX_NB_CUTS_PER_CUSTOMER (2)
// #define MIN_VIOLATION (0.1)
//
// struct ThreeVerticesVal
// {
//     Vertex i;
//     Vertex j;
//     Vertex k;
//     Float val;
// };
//
// SCIP_RETCODE subset_row_create_cut(
//     SCIP* scip,                  // SCIP
//     Problem& problem,            // Problem
//     SCIP_Sepa* sepa,             // Separator
//     const Vertex i,              // Customer 1 in the cut
//     const Vertex j,              // Customer 2 in the cut
//     const Vertex k,              // Customer 3 in the cut
//     SCIP_Result* result          // Output result
// )
// {
//     // Create constraint name.
// #ifdef DEBUG
//     auto name = fmt::format("subset_row({},{},{})",
//                             problem.instance->vertex_name[i],
//                             problem.instance->vertex_name[j],
//                             problem.instance->vertex_name[k]);
// #endif
//
//     // Create constraint.
//     SubsetRowCut cut(i, j, k
// #ifdef DEBUG
//         , std::move(name)
// #endif
//     );
//     SCIP_Row* row;
//     std::tie(*result, row) = problem.add_subset_row_cut(scip, std::move(cut), sepa);
//     debug_assert(row);
//
//     // Done.
//     return SCIP_OKAY;
// }
//
// // Separator
// static
// SCIP_RETCODE subset_row_separate(
//     SCIP* scip,            // SCIP
//     SCIP_Sepa* sepa,       // Separator
//     SCIP_RESULT* result    // Pointer to store the result of the separation call
// )
// {
//     using PathIndex = Count;
//
//     // Print.
//     debugln("Starting separator for subset row cuts at node {}, depth {}, obj {}:",
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
//     const auto& vars = problem.vars;
//     const auto& instance = *problem.instance;
//     const auto nb_customers = instance.nb_customers();
//
//     // Count the number of times each vertex is visted by each path.
//     Vector<HashMap<PathIndex, Count>> visit_counts(nb_customers);
//     for (PathIndex var_idx = 0; var_idx < static_cast<PathIndex>(vars.size()); ++var_idx)
//     {
//         const auto& [var, val, path, path_length] = vars[var_idx];
//         debug_assert(val == SCIPgetSolVal(scip, nullptr, var));
//         if (SCIPisPositive(scip, val))
//         {
//             for (PathLength idx = 1; idx < path_length - 1; ++idx)
//             {
//                 const auto vertex = path[idx];
//                 if (vertex < nb_customers)
//                 {
//                     visit_counts[vertex][var_idx]++;
//                 }
//             }
//         }
//     }
//
//     // Find the cuts.
//     Vector<ThreeVerticesVal> vertices_of_cuts;
//     for (Vertex i = 0; i < nb_customers; ++i)
//     {
//         for (Vertex j = i + 1; j < nb_customers; ++j)
//         {
//             auto ij_visit_counts = visit_counts[i];
//             for (const auto& [var_idx, count] : visit_counts[j])
//             {
//                 ij_visit_counts[var_idx] += count;
//             }
//
//             for (Vertex k = j + 1; k < nb_customers; ++k)
//             {
//                 // Get the number of visits by each path.
//                 auto ijk_visit_counts = ij_visit_counts;
//                 for (const auto& [var_idx, count] : visit_counts[k])
//                 {
//                     ijk_visit_counts[var_idx] += count;
//                 }
//
//                 // Calculate the LHS.
//                 Float lhs = 0;
//                 for (const auto& [var_idx, count] : ijk_visit_counts)
//                 {
//                     lhs += std::floor(static_cast<Float>(count) / 2.0) * vars[var_idx].val;
//                 }
//
//                 // Store the cut if violated.
//                 if (lhs > 1.0 + MIN_VIOLATION)
//                 {
//                     vertices_of_cuts.push_back({i, j, k, lhs});
//                     debugln("    Customers {}, {} and {} has LHS {}",
//                             instance.vertex_name[i],
//                             instance.vertex_name[j],
//                             instance.vertex_name[k],
//                             lhs);
//                 }
//             }
//         }
//     }
//     std::partial_sort(vertices_of_cuts.begin(),
//                       vertices_of_cuts.begin() + std::min<size_t>(vertices_of_cuts.size(), MAX_NB_CUTS),
//                       vertices_of_cuts.end(),
//                       [](const ThreeVerticesVal& a, const ThreeVerticesVal& b)
//                       {
//                           return a.val > b.val;
//                       });
//
//     // Create cuts for the largest violations. Limit number of cuts per customer.
//     Vector<Count> cuts_per_customer(nb_customers);
//     for (size_t idx = 0; idx < std::min<size_t>(vertices_of_cuts.size(), MAX_NB_CUTS); ++idx)
//     {
//         const auto& [i, j, k, lhs] = vertices_of_cuts[idx];
//
//         if (cuts_per_customer[i] < MAX_NB_CUTS_PER_CUSTOMER &&
//             cuts_per_customer[j] < MAX_NB_CUTS_PER_CUSTOMER &&
//             cuts_per_customer[k] < MAX_NB_CUTS_PER_CUSTOMER)
//         {
//             // Print.
//             debugln("    Creating subset row cut on customers {}, {} and {} with LHS {} in branch-and-bound node {}",
//                     instance.vertex_name[i],
//                     instance.vertex_name[j],
//                     instance.vertex_name[k],
//                     lhs,
//                     SCIPnodeGetNumber(SCIPgetCurrentNode(scip)));
//
//             // Create cut.
//             SCIP_CALL(subset_row_create_cut(scip, problem, sepa, i, j, k, result));
//
//             // Increment count.
//             cuts_per_customer[i]++;
//             cuts_per_customer[j]++;
//             cuts_per_customer[k]++;
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
// SCIP_DECL_SEPACOPY(sepaCopySubsetRow)
// {
//     // Check.
//     debug_assert(scip);
//     debug_assert(sepa);
//     debug_assert(strcmp(SCIPsepaGetName(sepa), SEPA_NAME) == 0);
//
//     // Include separator.
//     SCIP_CALL(SCIPincludeSepaSubsetRow(scip));
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
// SCIP_DECL_SEPAEXECLP(sepaExeclpSubsetRow)
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
// #ifdef SUBSET_ROW_CUTS_MAX_DEPTH
//     if (SCIPgetDepth(scip) <= SUBSET_ROW_CUTS_MAX_DEPTH)
// #endif
//     {
//         SCIP_CALL(subset_row_separate(scip, sepa, result));
//     }
//
//     // Done.
//     return SCIP_OKAY;
// }
// #pragma GCC diagnostic pop
//
// // Create separator for subset row cuts and include it in SCIP
// SCIP_RETCODE SCIPincludeSepaSubsetRow(
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
//                                    sepaExeclpSubsetRow,
//                                    nullptr,
//                                    nullptr));
//     debug_assert(sepa);
//
//     // Set callbacks.
//     SCIP_CALL(SCIPsetSepaCopy(scip, sepa, sepaCopySubsetRow));
//
//     // Done.
//     return SCIP_OKAY;
// }
//
// #endif
