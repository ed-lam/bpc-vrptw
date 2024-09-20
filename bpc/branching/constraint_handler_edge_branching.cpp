// #define PRINT_DEBUG

#include "branching/constraint_handler_edge_branching.h"
#include "output/formatting.h"
#include "problem/debug.h"
#include "problem/problem.h"

// Constraint handler properties
#define CONSHDLR_NAME          "edge_branching"
#define CONSHDLR_DESC          "Stores the edge branching decisions"
#define CONSHDLR_ENFOPRIORITY  0          // priority of the constraint handler for constraint enforcing
#define CONSHDLR_CHECKPRIORITY 9999999    // priority of the constraint handler for checking feasibility
#define CONSHDLR_PROPFREQ      1          // frequency for propagating domains; zero means only preprocessing propagation
#define CONSHDLR_EAGERFREQ     1          // frequency for using all instead of only the useful constraints in separation,
                                          // propagation and enforcement, -1 for no eager evaluations, 0 for first only
#define CONSHDLR_DELAYPROP false          // should propagation method be delayed, if other propagators found reductions?
#define CONSHDLR_NEEDSCONS true           // should the constraint handler be skipped, if no constraints are available?

#define CONSHDLR_PROP_TIMING SCIP_PROPTIMING_BEFORELP

// Constraint data
struct EdgeBranchingConsData
{
    SCIP_Node* node;         // The node of the branch-and-bound tree for this constraint
    Edge edge;               // Decision
    BranchDirection dir;     // Branch direction
    Size npropagatedvars;    // Number of variables that existed when the related node was propagated the last time.
                             // Used to determine whether the constraint should be repropagated
    Bool propagated;         // Is the constraint already propagated?
};

// Create constraint data
static SCIP_Retcode consdataCreate(
    SCIP* scip,                          // SCIP
    EdgeBranchingConsData** consdata,    // Pointer to the constraint data
    SCIP_Node* node,                     // Node of the branch-and-bound tree for this constraint
    const Edge edge,                     // Edge
    const BranchDirection dir            // Branch direction
)
{
    // Check.
    debug_assert(scip);
    debug_assert(consdata);

    // Create constraint data.
    SCIP_CALL(SCIPallocBlockMemory(scip, consdata));

    // Store data.
    (*consdata)->node = node;
    (*consdata)->edge = edge;
    (*consdata)->dir = dir;
    (*consdata)->npropagatedvars = 0;
    (*consdata)->propagated = false;

    // Done.
    return SCIP_OKAY;
}

Bool edge_branching_check_disable_path(
    const Edge edge,              // Decision
    const BranchDirection dir,    // Branch direction
    const Vector<Vertex>& path    // Path
)
{
    // If an edge must be used, disable the path if exactly one of the two vertices in the edge is used.
    // If an edge must not be used, disable the path if the edge is used.
    if (dir == BranchDirection::Forbid)
    {
        for (Size idx = 0; idx < path.size() - 1; ++idx)
        {
            const auto i = path[idx];
            const auto j = path[idx + 1];
            if (i == edge.i && j == edge.j)
            {
                return true;
            }
        }
    }
    else
    {
        if (edge.i == path[0])
        {
            // Disable if anywhere goes to j that is not i.
            for (Size idx = 0; idx < path.size() - 1; ++idx)
            {
                const auto i = path[idx];
                const auto j = path[idx + 1];
                if (i != edge.i && j == edge.j)
                {
                    return true;
                }
            }
        }
        else if (edge.j == path.back())
        {
            // Disable if i goes to anywhere that is not j.
            for (Size idx = 0; idx < path.size() - 1; ++idx)
            {
                const auto i = path[idx];
                const auto j = path[idx + 1];
                if (i == edge.i && j != edge.j)
                {
                    return true;
                }
            }
        }
        else
        {
            for (Size idx = 0; idx < path.size() - 1; ++idx)
            {
                const auto i = path[idx];
                const auto j = path[idx + 1];
                if ((i == edge.i) ^ (j == edge.j))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// Fix a variable to zero if its path is not valid for this constraint/branch
static inline SCIP_Retcode check_variable(
    SCIP* scip,                      // SCIP
    const Edge edge,                 // Decision
    const BranchDirection dir,       // Branch direction
    const PathVariable& path_var,    // Variable to check
    Size& nfixedvars,                // Pointer to store the number of fixed variables
    SCIP_Bool* cutoff                // Pointer to store if a cutoff was detected
)
{
    // Get the path variable.
    const auto& [var, _, path] = path_var;

    // Check.
    debug_assert(scip);
    debug_assert(cutoff);
    debug_assert(var);
    debug_assert(path.front() == reinterpret_cast<Problem*>(SCIPgetProbData(scip))->instance->depot());
    debug_assert(path.back() == reinterpret_cast<Problem*>(SCIPgetProbData(scip))->instance->depot());

    // If the variable is locally fixed to zero, continue to next variable.
    if (SCIPvarGetUbLocal(var) < 0.5)
    {
        return SCIP_OKAY;
    }

    // Disable the variable if its path doesn't satisfy the branching decision.
    if (edge_branching_check_disable_path(edge, dir, path))
    {
        // Disable the variable.
        SCIP_Bool success;
        SCIP_Bool fixing_is_infeasible;
        SCIP_CALL(SCIPfixVar(scip, var, 0.0, &fixing_is_infeasible, &success));

        // Print.
        debugln("            Disabling path {}", format_path(path));

        // Cut off the node if the fixing is infeasible.
        if (fixing_is_infeasible)
        {
            debug_assert(SCIPvarGetLbLocal(var) > 0.5);
            debugln("         Node is infeasible - cut off");
            (*cutoff) = true;
        }
        else
        {
            debug_assert(success);
            nfixedvars++;
        }
    }
    else
    {
        debugln("            Allowing path {}", format_path(path));
    }

    // Done.
    return SCIP_OKAY;
}

// Fix variables to zero if its path is not valid for this constraint/branching
static SCIP_Retcode fix_variables(
    SCIP* scip,                          // SCIP
    EdgeBranchingConsData* consdata,     // Constraint data
    const Vector<PathVariable>& vars,    // Array of variables
    SCIP_Result* result                  // Pointer to store the result of the fixing
)
{
    // Print.
    debugln("        Checking variables {} to {}:", consdata->npropagatedvars, vars.size());

    // Check.
    const auto nvars = vars.size();
    debug_assert(consdata->npropagatedvars <= nvars);

    // Check every variable.
    Size nfixedvars = 0;
    SCIP_Bool cutoff = false;
    for (Size v = consdata->npropagatedvars; v < nvars && !cutoff; ++v)
    {
        debug_assert(v < nvars);
        SCIP_CALL(check_variable(scip, consdata->edge, consdata->dir, vars[v], nfixedvars, &cutoff));
    }

    // Print.
    debugln("        Disabled {} variables", nfixedvars);

    // Done.
    if (cutoff)
    {
        *result = SCIP_CUTOFF;
    }
    else if (nfixedvars > 0)
    {
        *result = SCIP_REDUCEDDOM;
    }
    return SCIP_OKAY;
}

// Check if all variables are valid for the given constraint
#ifdef DEBUG
static void check_propagation(
    const Problem& problem,             // Problem
    EdgeBranchingConsData* consdata,    // Constraint data
    SCIP_Bool beforeprop                // Is this check performed before propagation?
)
{
    // Get variables.
    const auto& vars = problem.vars;
    const auto nvars = beforeprop ? consdata->npropagatedvars : vars.size();
    release_assert(nvars <= vars.size());

    // Get the branching decision.
    const auto edge = consdata->edge;
    const auto dir = consdata->dir;

    // Check that the path of every variable is feasible for this constraint.
    for (Size v = 0; v < nvars; ++v)
    {
        // Get variable.
        debug_assert(v < vars.size());
        const auto& [var, _, path] = vars[v];

        // If the variable is locally fixed to zero, continue.
        if (SCIPvarGetUbLocal(var) < 0.5)
        {
            continue;
        }

        // Check.
#ifdef DEBUG
        debug_assert(path.front() == problem.instance->depot());
        debug_assert(path.back() == problem.instance->depot());
#endif

        // Check.
        release_assert(!edge_branching_check_disable_path(edge, dir, path),
                       "Branching decision is not propagated correctly for path {}",
                       format_path(path));
    }
}
#endif

// Free constraint data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_CONSDELETE(consDeleteEdgeBranching)
{
    // Check.
    debug_assert(conshdlr);
    debug_assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
    debug_assert(consdata);
    debug_assert(*consdata);

    // Free memory.
    SCIPfreeBlockMemory(scip, reinterpret_cast<EdgeBranchingConsData**>(consdata));

    // Done.
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Transform constraint data into data belonging to the transformed problem
static SCIP_DECL_CONSTRANS(consTransEdgeBranching)
{
    // Check.
    debug_assert(conshdlr);
    debug_assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
    debug_assert(SCIPgetStage(scip) == SCIP_STAGE_TRANSFORMING);
    debug_assert(sourcecons);
    debug_assert(targetcons);

    // Get data of original constraint.
    auto sourcedata = reinterpret_cast<EdgeBranchingConsData*>(SCIPconsGetData(sourcecons));
    debug_assert(sourcedata);

    // Create constraint data for target constraint.
    EdgeBranchingConsData* targetdata;
    SCIP_CALL(consdataCreate(scip, &targetdata, sourcedata->node, sourcedata->edge, sourcedata->dir));
    debug_assert(targetdata);

    // Create target constraint.
    char name[SCIP_MAXSTRLEN];
    SCIPsnprintf(name, SCIP_MAXSTRLEN, "t_%s", SCIPconsGetName(sourcecons));
    SCIP_CALL(SCIPcreateCons(scip,
                             targetcons,
                             SCIPconsGetName(sourcecons),
                             conshdlr,
                             reinterpret_cast<SCIP_ConsData*>(targetdata),
                             SCIPconsIsInitial(sourcecons),
                             SCIPconsIsSeparated(sourcecons),
                             SCIPconsIsEnforced(sourcecons),
                             SCIPconsIsChecked(sourcecons),
                             SCIPconsIsPropagated(sourcecons),
                             SCIPconsIsLocal(sourcecons),
                             SCIPconsIsModifiable(sourcecons),
                             SCIPconsIsDynamic(sourcecons),
                             SCIPconsIsRemovable(sourcecons),
                             SCIPconsIsStickingAtNode(sourcecons)));

    // Done.
    return SCIP_OKAY;
}

// Domain propagation method of constraint handler
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_CONSPROP(consPropEdgeBranching)
{
    // Check.
    debug_assert(scip);
    debug_assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
    debug_assert(result);

    // Get problem.
    const auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));

    // Get variables.
    const auto& vars = problem.vars;

    // Propagate constraints.
    *result = SCIP_DIDNOTFIND;
    for (Size c = 0; c < nconss; ++c)
    {
        // Get constraint data.
        auto consdata = reinterpret_cast<EdgeBranchingConsData*>(SCIPconsGetData(conss[c]));
        debug_assert(consdata);

        // Check if all variables are valid for this constraint.
#ifdef DEBUG
        check_propagation(problem, consdata, true);
#endif

        // Propagate.
        if (!consdata->propagated)
        {
            // Print.
#ifdef PRINT_DEBUG
            {
                debugln("    Propagating edge branching constraint branch_{}({},{}) from node {} at depth {} in node {}"
                        " at depth {}",
                        consdata->dir,
                        consdata->edge.i,
                        consdata->edge.j,
                        SCIPnodeGetNumber(consdata->node),
                        SCIPnodeGetDepth(consdata->node),
                        SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
                        SCIPgetDepth(scip));
            }
#endif

            // Propagate.
            SCIP_CALL(fix_variables(scip, consdata, vars, result));

            // Set status.
            if (*result != SCIP_CUTOFF)
            {
                consdata->propagated = true;
                debug_assert(consdata->npropagatedvars <= static_cast<Size>(vars.size()));
                consdata->npropagatedvars = vars.size();
            }
            else
            {
                break;
            }
        }

        // Check if constraint is completely propagated.
#ifdef DEBUG
        check_propagation(problem, consdata, false);
#endif
    }

    // Done.
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Constraint activation notification method of constraint handler
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_CONSACTIVE(consActiveEdgeBranching)
{
    // Check.
    debug_assert(scip);
    debug_assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
    debug_assert(cons);

    // Get problem.
    const auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));

    // Get variables.
    const auto& vars = problem.vars;

    // Get constraint data.
    auto consdata = reinterpret_cast<EdgeBranchingConsData*>(SCIPconsGetData(cons));
    debug_assert(consdata);
    debug_assert(consdata->npropagatedvars <= vars.size());

    // Print.
    debugln("    Activating edge branching constraint branch_{}({},{}) from node {} at depth {} in node {} at depth {}",
            consdata->dir,
            consdata->edge.i,
            consdata->edge.j,
            SCIPnodeGetNumber(consdata->node),
            SCIPnodeGetDepth(consdata->node),
            SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
            SCIPgetDepth(scip));

    // Mark constraint as to be repropagated.
    if (consdata->npropagatedvars != vars.size())
    {
        consdata->propagated = false;
        SCIP_CALL(SCIPrepropagateNode(scip, consdata->node));
    }

    // Done.
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Constraint deactivation notification method of constraint handler
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_CONSDEACTIVE(consDeactiveEdgeBranching)
{
    // Check.
    debug_assert(scip);
    debug_assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
    debug_assert(cons);

    // Get the problem.
    const auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));

    // Get variables.
    const auto& vars = problem.vars;

    // Get constraint data.
    auto consdata = reinterpret_cast<EdgeBranchingConsData*>(SCIPconsGetData(cons));
    debug_assert(consdata);
    debug_assert(consdata->propagated || SCIPgetNChildren(scip) == 0);

    // Print.
    debugln("    Deactivating edge branching constraint branch_{}({},{})",
            consdata->dir,
            consdata->edge.i,
            consdata->edge.j);

    // Set the number of propagated variables to the current number of variables.
    consdata->npropagatedvars = vars.size();

    // Done.
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Create the constraint handler for a branch and include it
SCIP_Retcode SCIPincludeConshdlrEdgeBranching(
    SCIP* scip    // SCIP
)
{
    // Include constraint handler.
    SCIP_Conshdlr* conshdlr = nullptr;
    SCIP_CALL(SCIPincludeConshdlrBasic(scip,
                                       &conshdlr,
                                       CONSHDLR_NAME,
                                       CONSHDLR_DESC,
                                       CONSHDLR_ENFOPRIORITY,
                                       CONSHDLR_CHECKPRIORITY,
                                       CONSHDLR_EAGERFREQ,
                                       CONSHDLR_NEEDSCONS,
                                       nullptr,
                                       nullptr,
                                       nullptr,
                                       nullptr,
                                       nullptr));
    debug_assert(conshdlr);

    // Set callbacks.
    SCIP_CALL(SCIPsetConshdlrDelete(scip, conshdlr, consDeleteEdgeBranching));
    SCIP_CALL(SCIPsetConshdlrTrans(scip, conshdlr, consTransEdgeBranching));
    SCIP_CALL(SCIPsetConshdlrProp(scip,
                                  conshdlr,
                                  consPropEdgeBranching,
                                  CONSHDLR_PROPFREQ,
                                  CONSHDLR_DELAYPROP,
                                  CONSHDLR_PROP_TIMING));
    SCIP_CALL(SCIPsetConshdlrActive(scip, conshdlr, consActiveEdgeBranching));
    SCIP_CALL(SCIPsetConshdlrDeactive(scip, conshdlr, consDeactiveEdgeBranching));

    // Done.
    return SCIP_OKAY;
}

// Create and capture a constraint enforcing a branch
SCIP_Retcode SCIPcreateConsEdgeBranching(
    SCIP* scip,                   // SCIP
    SCIP_Cons** cons,             // Pointer to the created constraint
    const char* name,             // Name of constraint
    SCIP_Node* node,              // The node of the branch-and-bound tree for this constraint
    const Edge edge,              // Edge
    const BranchDirection dir,    // Branch direction
    SCIP_Bool local               // Is this constraint only valid locally?
)
{
    // Find the constraint handler.
    auto conshdlr = SCIPfindConshdlr(scip, CONSHDLR_NAME);
    debug_assert(conshdlr);

    // Create constraint data.
    EdgeBranchingConsData* consdata;
    SCIP_CALL(consdataCreate(scip, &consdata, node, edge, dir));
    debug_assert(consdata);

    // Create constraint.
    SCIP_CALL(SCIPcreateCons(scip,
                             cons,
                             name,
                             conshdlr,
                             reinterpret_cast<SCIP_ConsData*>(consdata),
                             false,
                             false,
                             false,
                             false,
                             true,
                             local,
                             false,
                             false,
                             false,
                             true));

    // Print.
    debugln("    Creating edge branching constraint branch_{}({},{}) in node {} at depth {} (parent {})",
            consdata->dir,
            consdata->edge.i,
            consdata->edge.j,
            SCIPnodeGetNumber(consdata->node),
            SCIPnodeGetDepth(consdata->node),
            SCIPnodeGetNumber(SCIPnodeGetParent(consdata->node)));

    // Done.
    return SCIP_OKAY;
}

// Get edge branching decision
EdgeBranchingDecision SCIPgetEdgeBranchingDecision(
    SCIP_Cons* cons    // Constraint enforcing edge branching
)
{
    debug_assert(cons);
    auto consdata = reinterpret_cast<EdgeBranchingConsData*>(SCIPconsGetData(cons));
    debug_assert(consdata);

    EdgeBranchingDecision decision;
    decision.edge = consdata->edge;
    decision.dir = consdata->dir;
    return decision;
}
