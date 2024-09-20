// #define PRINT_DEBUG

// #include <immintrin.h>
// #include <mimalloc.h>
#include "labeling/labeling_algorithm.h"
#include "output/formatting.h"
#include "problem/problem.h"
#include "types/bitset.h"
#include "types/float_compare.h"

LabelingAlgorithm::LabelingAlgorithm(const Instance& instance) :
    instance_(instance),
    reduced_cost_(instance.num_vertices(), instance.num_vertices()),
// #ifdef USE_SUBSET_ROW_CUTS
//     subset_row_cuts_vertices_(nullptr),
//     subset_row_cuts_duals_(nullptr),
//     nb_subset_row_cuts_(0),
// #endif

    storage_(),
    queue_(),
    pareto_frontier_(instance.num_customers()),
    obj_(0)

#ifdef DEBUG
  , verbose_(false),
    next_label_id_(0)
#endif
{
}

// #ifdef USE_SUBSET_ROW_CUTS
// void LabelingAlgorithm::store_subset_row_duals(
//     const Vector<ThreeVertices>& subset_row_cuts_vertices,
//     const Vector<Float>& subset_row_cuts_duals
// )
// {
//     debug_assert(subset_row_cuts_vertices.size() == subset_row_cuts_duals.size());
//
//     {
//         const auto size = subset_row_cuts_vertices.size();
//         subset_row_cuts_vertices_ = reinterpret_cast<ThreeVertices*>(
//             mi_realloc(subset_row_cuts_vertices_, size * sizeof(ThreeVertices)));
//         std::memcpy(subset_row_cuts_vertices_, subset_row_cuts_vertices.data(), size * sizeof(ThreeVertices));
//     }
//     {
//         auto size = subset_row_cuts_duals.size();
//         size += (8 - size % 8) % 8;
//         debug_assert(size % 8 == 0 && size >= subset_row_cuts_duals.size());
//         subset_row_cuts_duals_ = reinterpret_cast<Float*>(
//             mi_realloc_aligned(subset_row_cuts_duals_, size * sizeof(Float), 64));
//         debug_assert(reinterpret_cast<uintptr_t>(subset_row_cuts_duals_) % 64 == 0);
//         std::memcpy(subset_row_cuts_duals_, subset_row_cuts_duals.data(), subset_row_cuts_duals.size() * sizeof(Float));
//     }
//     nb_subset_row_cuts_ = static_cast<Count>(subset_row_cuts_vertices.size());
// }
// #endif

void LabelingAlgorithm::create_source_label()
{
    // Get the depot.
    const auto j = instance_.depot();

    // Create the new label.
    auto next = static_cast<Label*>(storage_.get_buffer());
    storage_.commit_buffer();
    memset(next, 0, storage_.object_size());
#ifdef DEBUG
    next->id = next_label_id_++;
#endif
    next->time = instance_.vertex_earliest[j];
    next->vertex = j;

    // Insert into the priority queue.
    queue_.push(next);

    // Print.
#ifdef DEBUG
    if (verbose_)
    {
        println("        Generating source label {} at {} (vertex {} ({}), cost {}, load {}, time {}, unreachable {})",
                next->id,
                fmt::ptr(next),
                j,
                instance_.vertex_name[j],
                next->cost,
                Load{next->load},
                next->time,
                format_bitset(next->bitsets, instance_.num_customers()));
    }
#endif
}

Label* LabelingAlgorithm::extend_to_customer(const Label* const __restrict current, const Vertex j)
{
    // Check.
    debug_assert(j < instance_.num_customers());
    debug_assert(!get_bitset(current->bitsets, j));

    // Get the instance.
    const auto num_customers = instance_.num_customers();
    const auto depot = instance_.depot();

    // Create the new label.
    auto next = static_cast<Label*>(storage_.get_buffer());
    memcpy(next, current, storage_.object_size());
#ifdef DEBUG
    next->id = next_label_id_++;
#endif
    next->parent = current;
    next->vertex = j;
    debug_assert(!next->dominated);

    // Calculate the resources.
    const auto i = current->vertex;
    next->cost += reduced_cost_(i, j);
    next->load += instance_.vertex_load[j];
    next->time = std::max<Time>(instance_.vertex_earliest[j], next->time + instance_.service_plus_travel(i, j));
    debug_assert(next->load <= instance_.vehicle_load_capacity);
    debug_assert(next->time <= instance_.vertex_latest[j]);
    debug_assert(next->time + instance_.service_plus_travel(j, depot) <= instance_.vertex_latest[depot]);

    // Update the unreachable customers.
    {
        auto next_unreachable = next->bitsets;
        for (Vertex k = 0; k < num_customers; ++k)
            if (!get_bitset(next_unreachable, k))
            {
                const auto load_j_k = next->load + instance_.vertex_load[k];
                const auto time_j_k = next->time + instance_.service_plus_travel(j, k);
                const auto time_j_k_depot = time_j_k + instance_.service_plus_travel(k, depot);

                if (load_j_k > instance_.vehicle_load_capacity ||
                    time_j_k > instance_.vertex_latest[k] ||
                    time_j_k_depot > instance_.vertex_latest[depot])
                {
                    set_bitset(next_unreachable, k);
                }
            }
    }

    // Update the resources for the subset row cuts.
// #ifdef USE_SUBSET_ROW_CUTS
//     {
//         const auto unreachable_size = (num_customers + CHAR_BIT - 1) / CHAR_BIT;
//         Byte* next_subset_row = next->bitsets + unreachable_size;
//         for (Count idx = 0; idx < nb_subset_row_cuts_; ++idx)
//         {
//             const auto& vertices = subset_row_cuts_vertices_[idx];
//             if (j == vertices.i || j == vertices.j || j == vertices.k)
//             {
//                 const auto val = flip_bitset(next_subset_row, idx);
//                 next->cost -= (!val) * subset_row_cuts_duals_[idx];
//             }
//         }
//     }
// #endif

    // Print.
#ifdef DEBUG
    if (verbose_)
    {
        println("        Generating customer label {} at {} (parent {}, vertex {} ({}), cost {}, load {}, time {}, "
                "unreachable {})",
                next->id,
                fmt::ptr(next),
                current->id,
                j,
                instance_.vertex_name[j],
                next->cost,
                Load{next->load},
                next->time,
                format_bitset(next->bitsets, instance_.num_customers()));
    }
#endif

    // Check dominance.
    if (pareto_frontier_[j].add_label(next))
    {
        // Commit label.
        storage_.commit_buffer();
    }
    else
    {
        // Discard label.
        next = nullptr;
    }

    // Done.
    return next;
}

Label* LabelingAlgorithm::extend_to_sink(const Label* const __restrict current)
{
    // Get the depot.
    const auto j = instance_.depot();

    // Create the new label.
    auto next = static_cast<Label*>(storage_.get_buffer());
    memcpy(next, current, storage_.object_size());
#ifdef DEBUG
    next->id = next_label_id_++;
#endif
    next->parent = current;
    next->vertex = j;
    debug_assert(!next->dominated);

    // Calculate the resources.
    const auto i = current->vertex;
    next->cost += reduced_cost_(i, j);
    debug_assert(instance_.vertex_load[j] == 0);
    next->time = std::max<Time>(instance_.vertex_earliest[j], next->time + instance_.service_plus_travel(i, j));
    debug_assert(next->time <= instance_.vertex_latest[j]);

    // Print.
#ifdef DEBUG
    if (verbose_)
    {
        println("        Generating sink label {} at {} (parent {}, vertex {} ({}), cost {}, load {}, time {}, "
                "unreachable {})",
                next->id,
                fmt::ptr(next),
                current->id,
                j,
                instance_.vertex_name[j],
                next->cost,
                Load{next->load},
                next->time,
                format_bitset(next->bitsets, instance_.num_customers()));
    }
#endif

    // Discard the label if it is worse than previously found paths.
    if (!is_lt(next->cost, obj_ / 1.3))
    {
        next = nullptr;
    }

    // Done.
    return next;
}

void LabelingAlgorithm::solve(
    SCIP* scip,                    // SCIP
    Problem& problem,              // Problem
    const Bool feasible_master,    // Indicates if the master problem is feasible
    SCIP_Result* result,           // Output result
    Cost* lower_bound              // Output lower bound
)
{
    // Turn on debug printing.
    // set_verbose();

    // Get instance.
    const auto num_customers = instance_.num_customers();
    const auto depot = instance_.depot();

    // Clear solver state.
    for (Vertex i = 0; i < num_customers; ++i)
    {
        pareto_frontier_[i].reset(unreachable_size());
    }
    storage_.reset(label_size());
    queue_.clear();
    obj_ = 0;

    // Create the starting label.
    create_source_label();

    // Main loop.
    Size num_new_paths = 0;
    Vector<Vertex> path;
    Size iter = 0;
    while (!(!feasible_master && num_new_paths >= 1) &&
           !(num_new_paths >= 2000)                  &&
           !(num_new_paths >= 100 && iter >= 1000)   &&
           !(num_new_paths >= 50  && iter >= 5000)   &&
           !(num_new_paths >= 20  && iter >= 10000)  &&
           !(num_new_paths >= 1   && iter >= 20000)  &&
           !SCIPisStopped(scip)                      &&
           !queue_.empty())
    {
        // Pop the priority queue.
        const auto current = queue_.top();
        const auto i = current->vertex;
        queue_.pop();

        // Print.
#ifdef DEBUG
        if (verbose_)
        {
            println("    Popped label {} at {} (vertex {} ({}), cost {}, load {}, time {}, unreachable {})",
                    current->id,
                    fmt::ptr(current),
                    i,
                    instance_.vertex_name[i],
                    current->cost,
                    Load{current->load},
                    current->time,
                    format_bitset(current->bitsets, instance_.num_customers()));
        }
#endif

        // Skip if dominated.
        if (current->dominated)
        {
            continue;
        }

        // Extend to customers.
        for (Vertex j = 0; j < num_customers; ++j)
            if (!std::isnan(reduced_cost_(i, j)) && !get_bitset(current->bitsets, j))
            {
                auto next = extend_to_customer(current, j);
                if (next)
                {
                    queue_.push(next);
                }
            }

        // Extend to the depot.
        if (!std::isnan(reduced_cost_(i, depot)))
        {
            auto next = extend_to_sink(current);
            if (next)
            {
                // Store the objective value.
                obj_ = std::min(obj_, next->cost);

                // Get the path and its cost.
                debug_assert(path.empty());
                for (const Label* label = next; label; label = label->parent)
                {
                    path.push_back(label->vertex);
                }
                std::reverse(path.begin(), path.end());
                debug_assert(path.size() >= 2);
                debug_assert(path.front() == depot);
                debug_assert(path.back() == depot);

                // Add the new path.
                ++num_new_paths;
                debugln("        Found {} paths with reduced cost {}: {}",
                        num_new_paths, next->cost, format_path(path));
                problem.add_priced_var(scip, std::move(path));
            }
        }

        // Exit if enough paths are generated.
        ++iter;
    }

#ifdef DEBUG
    if (verbose_)
    {
        debugln("");

        if (queue_.empty())
        {
            println("    Queue emptied");
            println("    Optimal objective value: {}", obj_);
        }
        println("    Number of new paths: {}", num_new_paths);
        println("    Number of iterations: {}", iter);
        println("    Number of extensions: {}", next_label_id_);
        // debugln("    Run time: {:.2f} seconds", get_clock(scip) - start_time);
    }
#endif

    // Set time out status.
    *result = num_new_paths > 0 || !SCIPisStopped(scip) ? SCIP_SUCCESS : SCIP_DIDNOTRUN;

    // Set lower bound.
    if (lower_bound && feasible_master && queue_.empty() && num_new_paths > 0)
    {
        *lower_bound = std::max(SCIPgetLPObjval(scip) + obj_ * instance_.num_customers(), 0.0);
#ifdef DEBUG
        if (verbose_)
        {
            println("    Computed lower bound {} in node {}",
                    *lower_bound, SCIPnodeGetNumber(SCIPgetCurrentNode(scip)));
        }
#endif
    }
}

#ifdef DEBUG
void LabelingAlgorithm::set_verbose(const bool on)
{
    verbose_ = on;
}
#endif
