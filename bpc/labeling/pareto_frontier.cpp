// #define PRINT_DEBUG

// #include <immintrin.h>
#include "labeling/labeling_algorithm.h"
#include "labeling/pareto_frontier.h"
#include "problem/debug.h"
#include "types/bitset.h"
#include <algorithm>
#include <cstdlib>

Bool bitset_dominates(const Byte* __restrict bitset_1, const Byte* __restrict bitset_2, const Size size)
{
    debug_assert(bitset_1 != bitset_2);

    Byte bit_gt = 0;
    const auto bitset_1_end = bitset_1 + size;
    while (!bit_gt && bitset_1 != bitset_1_end)
    {
        // bitset_1 > bitset_2
        bit_gt |= (*bitset_1) & ~(*bitset_2);

        ++bitset_1;
        ++bitset_2;
    }
    return !bit_gt;
}

// #ifdef USE_SUBSET_ROW_CUTS
// inline bool subset_row_cost_dominates(
//     const Float cost_1,
//     const Byte* subset_row_1,
//     const Float cost_2,
//     const Byte* subset_row_2,
//     const Float* subset_row_cuts_duals,
//     const Count subset_row_size
// )
// {
//     Float sum_of_sigma = 0;
//     const auto subset_row_1_end = subset_row_1 + subset_row_size;
//     while (subset_row_1 != subset_row_1_end)
//     {
//         const auto val = *subset_row_1 & ~(*subset_row_2);
// // #ifdef __AVX512F__
// //         debug_assert(reinterpret_cast<uintptr_t>(subset_row_cuts_duals) % 64 == 0);
// //         sum_of_sigma += _mm512_mask_reduce_add_pd(val, _mm512_load_pd(subset_row_cuts_duals));
// // #else
//         sum_of_sigma += ((val & (1 << 0)) != 0) * subset_row_cuts_duals[0] +
//                         ((val & (1 << 1)) != 0) * subset_row_cuts_duals[1] +
//                         ((val & (1 << 2)) != 0) * subset_row_cuts_duals[2] +
//                         ((val & (1 << 3)) != 0) * subset_row_cuts_duals[3] +
//                         ((val & (1 << 4)) != 0) * subset_row_cuts_duals[4] +
//                         ((val & (1 << 5)) != 0) * subset_row_cuts_duals[5] +
//                         ((val & (1 << 6)) != 0) * subset_row_cuts_duals[6] +
//                         ((val & (1 << 7)) != 0) * subset_row_cuts_duals[7];
// // #endif
//         ++subset_row_1;
//         ++subset_row_2;
//         subset_row_cuts_duals += 8;
//     }
//     return cost_1 - sum_of_sigma <= cost_2;
// }
// #endif

void ParetoFrontier::reset(const Size unreachable_size)
{
    frontier_.clear();
    unreachable_size_ = unreachable_size;
}

Bool ParetoFrontier::add_label(Label* __restrict new_label)
{
    // Get the new label.
    const auto new_cost = new_label->cost;
    const auto new_load = new_label->load;
    const auto new_time = new_label->time;
    const auto new_unreachable = new_label->bitsets;

    for (auto it = frontier_.begin(); it != frontier_.end(); )
    {
        // Get the existing label.
        const auto existing_label = *it;
        const auto existing_cost = existing_label->cost;
        const auto existing_load = existing_label->load;
        const auto existing_time = existing_label->time;
        const auto existing_unreachable = existing_label->bitsets;

        // Check if the existing label dominates the new label.
        const auto existing_dominates_new =
            existing_cost <= new_cost &&
            existing_load <= new_load &&
            existing_time <= new_time &&
            bitset_dominates(existing_unreachable, new_unreachable, unreachable_size_);
        if (existing_dominates_new)
        {
            debugln("                New label dominated");
            return false;
        }

        // Check if the new label dominates the existing label.
        const auto new_dominates_existing =
            new_cost <= existing_cost &&
            new_time <= existing_time &&
            new_load <= existing_load &&
            bitset_dominates(new_unreachable, existing_unreachable, unreachable_size_);
        if (new_dominates_existing)
        {
            // Mark as dominated.
            existing_label->dominated = true;

            // Delete the existing label from the frontier.
            *it = frontier_.back();
            frontier_.pop_back();
            debugln("                Existing label dominated");
        }
        else
        {
            ++it;
        }
    }

    // Not dominated.
    return true;
}
