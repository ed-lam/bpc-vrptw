#pragma once

#include "problem/debug.h"
#include "problem/scip.h"
#include "types/basic_types.h"
#include "types/edge.h"

// Pseudocosts of an edge
// struct PseudocostsData
// {
//     SCIP_Real down_pcost_sum;
//     SCIP_Real up_pcost_sum;
//     Count down_ntimes;
//     Count up_ntimes;
// };

// Direction for branching
enum BranchDirection : Bool
{
    Forbid = false,
    Require = true
};

// Decision
struct EdgeBranchingDecision
{
    Edge edge;
    BranchDirection dir;
};

// Create the branching rule and include it in SCIP
SCIP_Retcode SCIPincludeEdgeBranchrule(
    SCIP* scip    // SCIP
);

template<>
struct fmt::formatter<BranchDirection> : fmt::formatter<std::string_view>
{
    auto format(const BranchDirection dir, fmt::format_context& ctx) const
    {
        std::string_view str = "unknown";
        switch (dir)
        {
            case BranchDirection::Forbid:  str = "forbid";  break;
            case BranchDirection::Require: str = "require"; break;
        }
        return fmt::formatter<std::string_view>::format(str, ctx);
    }
};
