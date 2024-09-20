// #ifdef USE_ROUNDED_CAPACITY_CUTS
//
// #ifndef VRP_CVRPSEP_H
// #define VRP_CVRPSEP_H
//
// #include "problem/problem.h"
// #include "types/vector.h"
// #include <cvrpsep/cnstrmgr.h>
//
// struct CVRPSEP
// {
//     CnstrMgrPointer cmp;
//     CnstrMgrPointer cmp_old;
//
//     Vector<int> demand;
//
//     Vector<int> edge_tail;
//     Vector<int> edge_head;
//     Vector<double> edge_val;
//
//     // Constructors and destructor
//     CVRPSEP() = delete;
//     CVRPSEP(const Instance& instance);
//     CVRPSEP(const CVRPSEP&) = delete;
//     CVRPSEP(CVRPSEP&&) = delete;
//     CVRPSEP& operator=(const CVRPSEP&) = delete;
//     CVRPSEP& operator=(CVRPSEP&&) = delete;
//     ~CVRPSEP();
// };
//
// #endif
//
// #endif
