// #ifdef USE_SUBSET_ROW_CUTS
//
// #ifndef VRP_SUBSET_ROW_CUT_H
// #define VRP_SUBSET_ROW_CUT_H
//
// #include "problem/debug.h"
// #include "problem/scip.h"
// #include "types/string.h"
// #include "types/basic_types.h"
//
// class SubsetRowCut
// {
// #ifdef DEBUG
//     String name_;
// #endif
//     SCIP_Row* row_;
//     Vertex i_;
//     Vertex j_;
//     Vertex k_;
//
//   public:
//     // Constructors
//     SubsetRowCut(const Vertex i, const Vertex j, const Vertex k
// #ifdef DEBUG
//         , String&& name
// #endif
//     ) :
// #ifdef DEBUG
//         name_(std::move(name)),
// #endif
//         row_(nullptr),
//         i_(i),
//         j_(j),
//         k_(k)
//     {
//     }
//
//     // Getters
//     inline auto row() const { return row_; }
//     inline auto i() const { return i_; }
//     inline auto j() const { return j_; }
//     inline auto k() const { return k_; }
// #ifdef DEBUG
//     inline const auto& name() const { return name_; }
// #endif
//
//     // Setters
//     inline void set_row(SCIP_Row* row) { row_ = row; }
// };
//
// #endif
//
// #endif
