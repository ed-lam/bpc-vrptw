// #ifndef VRP_ROBUST_CUT_H
// #define VRP_ROBUST_CUT_H
//
// #include "problem/debug.h"
// #include "problem/scip.h"
// #include "types/edge.h"
// #include "types/string.h"
// #include "types/basic_types.h"
//
// class RobustCut
// {
// #ifdef DEBUG
//     String name_;
// #endif
//     SCIP_Cons* cons_;
//     SCIP_Row* row_;
//     Float rhs_;
//     Count nb_edges_;
//     EdgeVal* edges_;
//
//   public:
//     // Constructors
//     RobustCut(SCIP* scip, const Count nb_edges, const Float rhs
// #ifdef DEBUG
//         , String&& name
// #endif
//     ) :
// #ifdef DEBUG
//         name_(std::move(name)),
// #endif
//         cons_(nullptr),
//         row_(nullptr),
//         rhs_(rhs),
//         nb_edges_(nb_edges)
//     {
//         scip_assert(SCIPallocBlockMemoryArray(scip, &edges_, nb_edges));
//     }
//
//     // Getters
//     inline auto cons() const { return cons_; }
//     inline auto row() const { return row_; }
//     inline auto rhs() const { return rhs_; }
//     inline auto nb_edges() const { return nb_edges_; }
//     inline auto edges() const { return edges_; }
//     inline auto& edges(const Count idx) { return edges_[idx]; }
//     inline auto edges(const Count idx) const { return edges_[idx]; }
//     inline Float coeff(const Edge edge) const
//     {
//         Float val = 0;
//         for (auto it = begin(); it != end(); ++it)
//             if (it->i == edge.i && it->j == edge.j)
//             {
//                 val = it->val;
//                 return val;
//             }
//         return val;
//     }
// #ifdef DEBUG
//     inline const auto& name() const { return name_; }
// #endif
//
//     // Iterators
//     inline const EdgeVal* begin() const { return edges_; }
//     inline const EdgeVal* end() const { return edges_ + nb_edges_; }
//
//     // Setters
//     inline void set_cons(SCIP_Cons* cons) { cons_ = cons; }
//     inline void set_row(SCIP_Row* row) { row_ = row; }
// };
//
// #endif
