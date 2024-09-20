// #ifdef USE_TWO_PATH_CUTS
//
// // #define PRINT_DEBUG
//
// #include "inequalities/two_path_labeling_algorithm.h"
// #include "output/formatting.h"
// #include "problem/debug.h"
// #include "types/array.h"
// #include "types/bitset.h"
// #include <immintrin.h>
//
// union Int16x16
// {
//     __m256i data;
//     Int16 array[16];
// };
// static_assert(sizeof(Int16x16) == 32);
// static_assert(alignof(Int16x16) == 32);
//
// struct SuccessorsData
// {
//     Int16x16 vertex;
//     Int16x16 travel;
//     Int16x16 earliest;
//     Int16x16 latest;
//     Int16x16 unreachable;
//     Int16x16 vertex_bitmask;
//     Count size;
// };
//
// bool TwoPathLabelingAlgorithm::solve(const TwoPathCutsVerticesSubset& vertices_in_cut)
// {
//     // Create output.
//     bool feasible = false;
//
//     // Print.
//     debugln("    Starting two-path TSPTW labeling algorithm");
//
//     // Calculate the size of the problem.
//     const auto depot = 0;
//     auto nb_customers = 0;
//     for (; nb_customers < vertices_in_cut.size(); ++nb_customers)
//         if (vertices_in_cut[nb_customers] >= instance_.nb_customers())
//         {
//             break;
//         }
//     const auto nb_vertices = nb_customers + 1;
// #ifdef DEBUG
//     for (Count idx = nb_customers; idx < vertices_in_cut.size(); ++idx)
//     {
//         debug_assert(vertices_in_cut[nb_customers] >= instance_.nb_customers());
//     }
// #endif
//
//     // Retrieve the transition data.
//     Array<SuccessorsData, TWO_PATH_CUTS_MAX_SUBSET_SIZE> successors_data;
//     static_assert(TWO_PATH_CUTS_MAX_SUBSET_SIZE <= 16,
//         "Maximum size for the set of vertices in the two-path separator must be less than or equal to 16");
//     {
//         const auto i = depot;
//         const auto ii = instance_.depot();
//
//         Count idx = 0;
//         for (Count j = 1; j <= nb_customers; ++j)
//         {
//             const auto jj = vertices_in_cut[j - 1];
//
//             if (instance_.is_valid(ii, jj))
//             {
//                 successors_data[i].vertex.array[idx] = j;
//                 successors_data[i].travel.array[idx] = instance_.service_plus_travel(ii, jj);
//                 successors_data[i].earliest.array[idx] = instance_.vertex_earliest[jj];
//                 successors_data[i].latest.array[idx] = instance_.vertex_latest[jj];
//                 successors_data[i].unreachable.array[idx] = (1 << j);
//                 for (Count k = 1; k <= nb_customers; ++k)
//                 {
//                     const auto kk = vertices_in_cut[k - 1];
//                     if (instance_.vertex_earliest[jj] + instance_.service_plus_travel(jj, kk) > instance_.vertex_latest[kk])
//                     {
//                         successors_data[i].unreachable.array[idx] |= (1 << k);
//                     }
//                 }
//                 successors_data[i].vertex_bitmask.array[idx] = (1 << j);
//                 ++idx;
//             }
//         }
//         successors_data[i].size = idx;
//     }
//     for (Count i = 1; i <= nb_customers; ++i)
//     {
//         const auto ii = vertices_in_cut[i - 1];
//
//         Count idx = 0;
//         {
//             const auto j = depot;
//             const auto jj = instance_.depot();
//
//             if (instance_.is_valid(ii, jj))
//             {
//                 successors_data[i].vertex.array[idx] = j;
//                 successors_data[i].travel.array[idx] = instance_.service_plus_travel(ii, jj);
//                 successors_data[i].earliest.array[idx] = instance_.vertex_earliest[jj];
//                 successors_data[i].latest.array[idx] = instance_.vertex_latest[jj];
//                 successors_data[i].unreachable.array[idx] = (1 << j);
//                 successors_data[i].vertex_bitmask.array[idx] = (1 << j);
//                 ++idx;
//             }
//         }
//         for (Count j = 1; j <= nb_customers; ++j)
//         {
//             const auto jj = vertices_in_cut[j - 1];
//
//             if (instance_.is_valid(ii, jj))
//             {
//                 successors_data[i].vertex.array[idx] = j;
//                 successors_data[i].travel.array[idx] = instance_.service_plus_travel(ii, jj);
//                 successors_data[i].earliest.array[idx] = instance_.vertex_earliest[jj];
//                 successors_data[i].latest.array[idx] = instance_.vertex_latest[jj];
//                 successors_data[i].unreachable.array[idx] = (1 << j);
//                 for (Count k = 1; k <= nb_customers; ++k)
//                 {
//                     const auto kk = vertices_in_cut[k - 1];
//                     if (instance_.vertex_earliest[jj] + instance_.service_plus_travel(jj, kk) > instance_.vertex_latest[kk])
//                     {
//                         successors_data[i].unreachable.array[idx] |= (1 << k);
//                     }
//                 }
//                 successors_data[i].vertex_bitmask.array[idx] = (1 << j);
//                 ++idx;
//             }
//         }
//         successors_data[i].size = idx;
//     }
//
//     // Create the root label.
//     queue_.clear();
//     queue_.push({depot, 0, 0, 0});
//
//     // Create mapping from new vertex index to original vertex index.
// #ifdef PRINT_DEBUG
//     Vector<Vertex> map_vertex_index(nb_vertices);
//     map_vertex_index.front() = instance_.depot(); // Depot
//     std::copy(vertices_in_cut.begin(), vertices_in_cut.begin() + nb_customers, map_vertex_index.begin() + 1);
// #endif
//
//     // Extend.
//     while (!queue_.empty())
//     {
//         // Get a label to extend.
//         const auto label = queue_.top();
//         queue_.pop();
//         debugln("        Extending label containing vertex {} ({}), cost {}, time {}, unreachable {}",
//                 label.vertex,
//                 instance_.vertex_name[map_vertex_index[label.vertex]],
//                 label.cost,
//                 label.time,
//                 format_bitset(&label.unreachable, nb_vertices));
//
//         // Get successors.
//         const auto& successors = successors_data[label.vertex];
//
//         // Extend cost.
//         const auto label_cost = _mm256_set1_epi16(label.cost);
//         const auto extension_cost = _mm256_set1_epi16(1);
//         const auto new_labels_cost = _mm256_add_epi16(label_cost, extension_cost);
//
//         // Extend time.
//         const auto label_time = _mm256_set1_epi16(label.time);
//         const auto extension_time = successors.travel.data;
//         const auto new_labels_time = _mm256_max_epi16(_mm256_add_epi16(label_time, extension_time), successors.earliest.data);
//
//         // Extend unreachable.
//         const auto label_unreachable = _mm256_set1_epi16(label.unreachable);
//         const auto extension_unreachable = successors.unreachable.data;
//         const auto new_labels_unreachable = _mm256_or_si256(label_unreachable, extension_unreachable);
//
//         // Determine if the new labels are infeasible.
// //        const auto time_infeasible = _mm256_cmpgt_epi16_mask(new_labels_time, successors.latest.data);
// //        const auto unreachable_infeasible = _mm256_test_epi16_mask(label_unreachable, successors.vertex_bitmask.data);
// //        const auto infeasible = time_infeasible | unreachable_infeasible;
// //        debugln("            New labels infeasible: {}", format_bitset(&infeasible, successors.size));
//         const auto time_infeasible2 = _mm256_cmpgt_epi16(new_labels_time, successors.latest.data);
//         const auto unreachable_infeasible2 = _mm256_and_si256(label_unreachable, successors.vertex_bitmask.data);
//         const auto infeasible2 = _mm256_or_si256(time_infeasible2, unreachable_infeasible2);
// //        for (Int i = 0; i < 16; ++i)
// //        {
// //            debug_assert((((Int16*)&infeasible2)[i] != 0) == get_bitset(&infeasible, i));
// //        }
//
//         // Add new labels into the queue.
//         for (auto idx = 0; idx < successors.size; ++idx)
//         {
//             if (reinterpret_cast<const Int16*>(&infeasible2)[idx] == 0)
//             {
//                 // Retrieve the new label.
//                 TwoPathLabel new_label{successors.vertex.array[idx],
//                                        reinterpret_cast<const Int16x16*>(&new_labels_cost)->array[idx],
//                                        reinterpret_cast<const Int16x16*>(&new_labels_time)->array[idx],
//                                        reinterpret_cast<const Int16x16*>(&new_labels_unreachable)->array[idx]};
//                 debugln("            Feasible new label: vertex {} ({}), cost {}, time {}, unreachable {}",
//                         new_label.vertex,
//                         instance_.vertex_name[map_vertex_index[new_label.vertex]],
//                         new_label.cost,
//                         new_label.time,
//                         format_bitset(&new_label.unreachable, nb_vertices));
//
//                 // Return feasible or continue.
//                 if (new_label.vertex == depot)
//                 {
//                     if (new_label.cost == nb_vertices)
//                     {
//                         feasible = true;
//                         goto EXIT;
//                     }
//                 }
//                 else
//                 {
//                     queue_.push(new_label);
//                 }
//             }
// #ifdef PRINT_DEBUG
//             else
//             {
//                 TwoPathLabel new_label{successors.vertex.array[idx],
//                                        reinterpret_cast<const Int16x16*>(&new_labels_cost)->array[idx],
//                                        reinterpret_cast<const Int16x16*>(&new_labels_time)->array[idx],
//                                        reinterpret_cast<const Int16x16*>(&new_labels_unreachable)->array[idx]};
//                 debugln("            Infeasible new label: vertex {} ({}), cost {}, time {}, unreachable {}",
//                         new_label.vertex,
//                         instance_.vertex_name[map_vertex_index[new_label.vertex]],
//                         new_label.cost,
//                         new_label.time,
//                         format_bitset(&new_label.unreachable, nb_vertices));
//             }
// #endif
//         }
//         debugln("");
//     }
//
//     // Exit.
//     EXIT:
//     debugln("        TSPTW {}", feasible ? "feasible" : "infeasible");
//     return feasible;
// }
//
// #endif
