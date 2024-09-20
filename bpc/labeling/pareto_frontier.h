#pragma once

#include "types/basic_types.h"
#include "types/vector.h"

struct Label;

class ParetoFrontier
{
    Vector<Label*> frontier_;
    Size unreachable_size_;

  public:
    // Constructors and destructor
    ParetoFrontier() = default;
    ParetoFrontier(const ParetoFrontier&) = delete;
    ParetoFrontier(ParetoFrontier&&) = delete;
    ParetoFrontier& operator=(const ParetoFrontier&) = delete;
    ParetoFrontier& operator=(ParetoFrontier&&) = delete;
    ~ParetoFrontier() = default;

    // Modify
// #ifdef USE_SUBSET_ROW_CUTS
//     void reset(const Count nb_subset_row_cuts, const Float* const subset_row_cuts_duals);
// #else
    void reset(const Size unreachable_size);
// #endif

    // Query methods
    Bool add_label(Label* __restrict new_label);
};
