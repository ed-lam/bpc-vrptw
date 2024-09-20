#pragma once

#include "branching/edge_branching.h"
#include "problem/scip.h"
#include "types/basic_types.h"
#include "types/edge.h"
#include "types/vector.h"

// Create the constraint handler for a branch and include it
SCIP_Retcode SCIPincludeConshdlrEdgeBranching(
    SCIP* scip    // SCIP
);

// Create and capture a constraint enforcing a branch
SCIP_Retcode SCIPcreateConsEdgeBranching(
    SCIP* scip,                   // SCIP
    SCIP_Cons** cons,             // Pointer to the created constraint
    const char* name,             // Name of constraint
    SCIP_Node* node,              // The node of the branch-and-bound tree for this constraint
    const Edge edge,              // Edge
    const BranchDirection dir,    // Branch direction
    SCIP_Bool local               // Is this constraint only valid locally?
);

// Get edge branching decision
EdgeBranchingDecision SCIPgetEdgeBranchingDecision(
    SCIP_Cons* cons    // Constraint enforcing edge branching
);

// Check if a path should be diabled according to a branching decision
Bool edge_branching_check_disable_path(
    const Edge edge,              // Decision
    const BranchDirection dir,    // Branch direction
    const Vector<Vertex>& path    // Path
);
