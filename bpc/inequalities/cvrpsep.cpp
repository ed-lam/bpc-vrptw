// #ifdef USE_ROUNDED_CAPACITY_CUTS
//
// #include "inequalities/cvrpsep.h"
//
// CVRPSEP::CVRPSEP(const Instance& instance) :
//     cmp(),
//     cmp_old(),
//     demand(),
//     edge_tail(),
//     edge_head(),
//     edge_val()
// {
//     // Create memory for storing the cuts.
//     CMGR_CreateCMgr(&cmp, 100);
//     CMGR_CreateCMgr(&cmp_old, 100);
//
//     // Get instance.
//     const auto nb_customers = instance.nb_customers();
//     const auto& vertex_load = instance.vertex_load;
//
//     // Copy load.
//     demand.resize(nb_customers);
//     for (Vertex i = 0; i < nb_customers; ++i)
//     {
//         demand[i] = vertex_load[i];
//     }
// }
//
// CVRPSEP::~CVRPSEP()
// {
//     debug_assert(cmp);
//     debug_assert(cmp_old);
//
//     CMGR_FreeMemCMgr(&cmp);
//     CMGR_FreeMemCMgr(&cmp_old);
// }
//
// #endif
