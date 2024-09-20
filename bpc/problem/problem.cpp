// #define PRINT_DEBUG

// #include "inequalities/cvrpsep.h"
// #include "inequalities/separator_rounded_capacity.h"
// #include "inequalities/separator_subset_row.h"
// #include "inequalities/separator_two_path.h"
// #include "inequalities/two_path_labeling_algorithm.h"
// #include "pricer/pricer_heuristic.h"
// #include "types/hash_map.h"
#include "branching/constraint_handler_edge_branching.h"
#include "branching/edge_branching.h"
#include "output/formatting.h"
#include "pricers/pricer_labeling.h"
#include "problem/problem.h"
#include "problem/scip.h"
#include <scip/cons_linear.h>

// Create problem data for transformed problem
static SCIP_DECL_PROBTRANS(prob_trans)
{
    // Check.
    debug_assert(scip);
    debug_assert(sourcedata);
    debug_assert(targetdata);

    // Create transformed problem.
    const auto& problem = *reinterpret_cast<Problem*>(sourcedata);
    *targetdata = reinterpret_cast<SCIP_ProbData*>(problem.transform(scip));
    return SCIP_OKAY;
}

// Free when finishing branch-and-bound
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static SCIP_DECL_PROBEXITSOL(prob_exitsol)
{
    // Check.
    debug_assert(scip);
    debug_assert(probdata);

    // Finish solve.
    auto& problem = *reinterpret_cast<Problem*>(probdata);
    problem.finish_solve(scip);
    return SCIP_OKAY;
}
#pragma GCC diagnostic pop

// Free memory of original problem
static SCIP_DECL_PROBDELORIG(prob_delorig)
{
    // Check.
    debug_assert(scip);
    debug_assert(probdata);

    // Free.
    auto& problem = **reinterpret_cast<Problem**>(probdata);
    problem.free(scip);
    return SCIP_OKAY;
}

// Free problem data of transformed problem
static SCIP_DECL_PROBDELTRANS(prob_deltrans)
{
    // Check.
    debug_assert(scip);
    debug_assert(probdata);

    // Free.
    auto& problem = **reinterpret_cast<Problem**>(probdata);
    problem.free(scip);
    return SCIP_OKAY;
}

// Create a problem
Problem::Problem(SCIP* scip, const SharedPtr<Instance>& instance) :
    instance(instance),
    vars(),
    fractional_edges(),
    customer_cover_conss(instance->num_customers()),
    edge_branching_conshdlr(nullptr)
{
    // Check.
    debug_assert(scip);

    // Create problem.
    scip_assert(SCIPcreateProbBasic(scip, instance->name.c_str()));

    // Set optimization direction.
    scip_assert(SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE));

    // Tell SCIP that the objective value will be always integral.
    scip_assert(SCIPsetObjIntegral(scip));

    // Create the customer cover constraints.
    const auto num_customers = instance->num_customers();
    for (Vertex i = 0; i < num_customers; ++i)
    {
        // Create constraint.
        const auto name = fmt::format("customer_cover({})", instance->vertex_name[i]);
        auto& cons = customer_cover_conss[i];
        scip_assert(SCIPcreateConsLinear(scip,
                                         &cons,
                                         name.c_str(),
                                         0,
                                         nullptr,
                                         nullptr,
                                         1.0,
                                         1.0,
                                         true,
                                         true,
                                         true,
                                         true,
                                         true,
                                         false,
                                         true,
                                         false,
                                         false,
                                         false));
        debug_assert(cons);

        // Add the constraint to the problem.
        scip_assert(SCIPaddCons(scip, cons));
    }

    // Include separator for rounded capacity cuts.
// #ifdef USE_ROUNDED_CAPACITY_CUTS
//     cvrpsep = std::make_shared<CVRPSEP>(*instance);
//     scip_assert(SCIPincludeSepaRoundedCapacity(scip));
// #endif

    // Include separator for two-path cuts.
// #ifdef USE_TWO_PATH_CUTS
//     one_path_checked = std::make_shared<HashMap<TwoPathCutsVerticesSubset, Bool>>();
//     two_path_labeling_algorithm = std::make_shared<TwoPathLabelingAlgorithm>(*instance);
//     scip_assert(SCIPincludeSepaTwoPath(scip));
// #endif

    // Include separator for subset row cuts.
// #ifdef USE_SUBSET_ROW_CUTS
//     scip_assert(SCIPincludeSepaSubsetRow(scip));
// #endif

    // Create the heuristic pricer.
// #ifdef USE_HEURISTIC_PRICING
//     scip_assert(SCIPincludePricerHeuristic(scip));
// #endif

    // Create the labeling pricer.
    scip_assert(SCIPincludePricerLabeling(scip, *instance));

    // Include the edge branching rule.
    scip_assert(SCIPincludeEdgeBranchrule(scip));
    scip_assert(SCIPincludeConshdlrEdgeBranching(scip));
    edge_branching_conshdlr = SCIPfindConshdlr(scip, "edge_branching");
    release_assert(edge_branching_conshdlr, "Constraint handler for edge branching is missing");
}

// Create an original problem
void Problem::create(SCIP* scip, const SharedPtr<Instance>& instance)
{
    // Create problem.
    Problem* problem = nullptr;
    scip_assert(SCIPallocBlockMemory(scip, &problem));
    debug_assert(problem);
    new(problem) Problem(scip, instance);

    // Set problem data.
    scip_assert(SCIPsetProbData(scip, reinterpret_cast<SCIP_ProbData*>(problem)));

    // Add callbacks.
    scip_assert(SCIPsetProbTrans(scip, prob_trans));
    scip_assert(SCIPsetProbExitsol(scip, prob_exitsol));
    scip_assert(SCIPsetProbDelorig(scip, prob_delorig));
    scip_assert(SCIPsetProbDeltrans(scip, prob_deltrans));
}

// Create a transformed problem
Problem* Problem::transform(SCIP* scip) const
{
    // Create transformed problem.
    Problem* transformed_problem = nullptr;
    scip_assert(SCIPallocBlockMemory(scip, &transformed_problem));
    debug_assert(transformed_problem);
    new(transformed_problem) Problem(*this);

    // Allocate memory for variables.
    release_assert(transformed_problem->vars.empty(), "Original problem already has variables");
    transformed_problem->vars.reserve(10000);

    // Transform customer cover constraints.
    scip_assert(SCIPtransformConss(scip,
                                   transformed_problem->customer_cover_conss.size(),
                                   transformed_problem->customer_cover_conss.data(),
                                   transformed_problem->customer_cover_conss.data()));

    // Transform robust cuts.
//     transformed_problem->robust_cuts.clear();
//     transformed_problem->robust_cuts.reserve(5000);
//     for (const auto& cut : robust_cuts)
//     {
//         // Create a new robust cut object.
//         auto& new_cut = transformed_problem->robust_cuts.emplace_back(scip,
//                                                                       cut.nb_edges(),
//                                                                       cut.rhs()
// #ifdef DEBUG
//                                                                     , "t_" + cut.name()
// #endif
//                                                                       );
//         std::copy(cut.edges(), cut.edges() + cut.nb_edges(), new_cut.edges());
//
//         // Transform the constraint.
//         auto cons = cut.cons();
//         release_assert(cons, "No constraint associated with robust cut while transforming problem");
//         scip_assert(SCIPtransformCons(scip, cons, &cons));
//         new_cut.set_cons(cons);
//     }

    // Done.
    return transformed_problem;
}

// Deallocate memory after branch-and-bound
void Problem::finish_solve(SCIP* scip)
{
    // Free rows of the robust cuts.
//     for (auto& cut : robust_cuts)
//     {
//         auto row = cut.row();
//         if (row)
//         {
//             scip_assert(SCIPreleaseRow(scip, &row));
//
//             auto edges = cut.edges();
//             SCIPfreeBlockMemoryArray(scip, &edges, cut.nb_edges());
//         }
//     }

    // Free rows of subset row cuts.
// #ifdef USE_SUBSET_ROW_CUTS
//     for (auto& cut : subset_row_cuts)
//     {
//         auto row = cut.row();
//         debug_assert(row);
//         scip_assert(SCIPreleaseRow(scip, &row));
//     }
// #endif
}

// Deallocate memory at exit
void Problem::free(SCIP* scip)
{
    // Free variables.
    for (auto& [var, _, __] : vars)
    {
        scip_assert(SCIPreleaseVar(scip, &var));
    }

    // Free customer cover constraints.
    for (auto& cons : customer_cover_conss)
    {
        scip_assert(SCIPreleaseCons(scip, &cons));
    }

    // Free constraints of the robust cuts.
//     for (auto& cut : robust_cuts)
//     {
//         auto cons = cut.cons();
//         if (cons)
//         {
//             scip_assert(SCIPreleaseCons(scip, &cons));
//
//             auto edges = cut.edges();
//             SCIPfreeBlockMemoryArray(scip, &edges, cut.nb_edges());
//         }
//     }

    // Free problem.
    auto problem = this;
    problem->~Problem();
    SCIPfreeBlockMemory(scip, &problem);
}

// Add a variable in pricing
void Problem::add_priced_var(SCIP* scip, Vector<Vertex>&& path_input)
{
    // Check.
    debug_assert(scip);
    debug_assert(path_input.size() >= 2);
    debug_assert(path_input.front() == instance->depot());
    debug_assert(path_input.back() == instance->depot());

    // Check that the path satisifies the branching decisions
#ifdef DEBUG
    {
        auto edge_branching_conss = SCIPconshdlrGetConss(edge_branching_conshdlr);
        const auto num_edge_branching_conss = SCIPconshdlrGetNConss(edge_branching_conshdlr);
        for (Size idx = 0; idx < num_edge_branching_conss; ++idx)
        {
            // Get the constraint.
            auto cons = edge_branching_conss[idx];
            debug_assert(cons);

            // Ignore constraints that are not active since these are not on the current active path of the search tree.
            if (!SCIPconsIsActive(cons))
            {
                continue;
            }

            // Check the decision.
            const auto [edge, dir] = SCIPgetEdgeBranchingDecision(cons);
            release_assert(!edge_branching_check_disable_path(edge, dir, path_input),
                           "Adding path {} that does not satisfy branching decision {} ({},{})",
                           format_path(path_input), dir, edge.i, edge.j);
        }
    }
#endif

    // Check that the path doesn't already exist.
#ifdef DEBUG
    for (const auto& [existing_var, _, existing_path] : vars)
    {
        release_assert(path_input != existing_path,
                       "Path {} already exists with value {}, lower bound {}, upper bound {}",
                       format_path(path_input),
                       SCIPgetSolVal(scip, nullptr, existing_var),
                       SCIPvarGetLbLocal(existing_var),
                       SCIPvarGetUbLocal(existing_var));
    }
#endif

    // Check that the path has no duplicate vertices.
// #ifndef USE_NG_ROUTE_PRICING
// #ifdef DEBUG
//     for (Size idx1 = 1; idx1 < path_length; ++idx1)
//         for (Size idx2 = idx1 + 1; idx2 < path_length; ++idx2)
//         {
//             release_assert(path[idx1] != path[idx2],
//                            "Path {} has duplicate vertex {}",
//                            format_path_with_names(path, path_length, instance->vertex_name),
//                            path[idx1]);
//         }
// #endif
// #endif

    // Calculate the cost.
    SCIP_Real cost = 0;
    for (Size idx = 0; idx < path_input.size() - 1; ++idx)
    {
        cost += instance->cost(path_input[idx], path_input[idx + 1]);
    }

    // Create variable data.
    auto& [var, val, path] = vars.emplace_back();

    // Create and add variable.
    const auto name = fmt::format("path({})", format_path_with_names(path_input, instance->vertex_name, ","));
    scip_assert(SCIPcreateVar(scip,
                              &var,
                              name.c_str(),
                              0.0,
                              1.0,
                              cost,
                              SCIP_VARTYPE_BINARY,
                              false,
                              true,
                              nullptr,
                              nullptr,
                              nullptr,
                              nullptr,
                              nullptr));
    debug_assert(var);
    scip_assert(SCIPaddPricedVar(scip, var, 1.0));

    // Print.
    debugln("            Adding column with cost {:.0f}, path {}",
            cost, format_path_with_names(path_input, instance->vertex_name));

    // Change the upper bound of the binary variable to lazy since the upper bound is
    // already enforced due to the objective function the set covering constraint. The
    // reason for doing is that, is to avoid the bound of x <= 1 in the LP relaxation
    // since this bound constraint would produce a dual variable which might have a
    // positive reduced cost.
    scip_assert(SCIPchgVarUbLazy(scip, var, 1.0));

    // Add column to customer cover constraints.
    const auto num_customers = instance->num_customers();
    {
        Vector<SCIP_Real> coeffs(num_customers);
        for (Size idx = 1; idx < path_input.size() - 1; ++idx)
        {
            const auto i = path_input[idx];
            debug_assert(0 <= i && i < num_customers);
            coeffs[i]++;
        }
        for (Vertex i = 0; i < num_customers; ++i)
            if (coeffs[i])
            {
                scip_assert(SCIPaddCoefLinear(scip, customer_cover_conss[i], var, coeffs[i]));
            }
    }

    // Add coefficients to the robust cuts.
//     for (const auto& cut : robust_cuts)
//     {
//         // Calculate the coefficient.
//         SCIP_Real coeff = 0.0;
//         for (PathLength idx = 0; idx < path_length - 1; ++idx)
//         {
//             const Edge edge{path[idx], path[idx + 1]};
//             coeff += cut.coeff(edge);
//         }
//
//         // Add variable to the cut.
//         if (coeff)
//         {
//             auto cons = cut.cons();
//             if (cons)
//             {
//                 scip_assert(SCIPaddCoefLinear(scip, cons, var, coeff));
//             }
//             else
//             {
//                 auto row = cut.row();
//                 debug_assert(row);
//                 scip_assert(SCIPaddVarToRow(scip, row, var, coeff));
//             }
//         }
//     }

    // Add coefficients to the subset row cuts.
// #ifdef USE_SUBSET_ROW_CUTS
//     for (const auto& cut : subset_row_cuts)
//     {
//         // Calculate the coefficient.
//         SCIP_Real coeff = 0.0;
//         for (PathLength idx = 1; idx < path_length - 1; ++idx)
//         {
//             const auto vertex = path[idx];
//             coeff += (vertex == cut.i()) || (vertex == cut.j()) || (vertex == cut.k());
//         }
//         coeff = std::floor(coeff / 2);
//
//         // Add variable to the cut.
//         if (coeff)
//         {
//             auto row = cut.row();
//             debug_assert(row);
//             scip_assert(SCIPaddVarToRow(scip, row, var, coeff));
//         }
//     }
// #endif

    // Store the path.
    path = std::move(path_input);
}

// Add a new robust constraint
// Pair<SCIP_Result, SCIP_Cons*> Problem::add_robust_constraint(
//     SCIP* scip,         // SCIP
//     RobustCut&& cut,    // Data for the cut
//     SCIP_Node* node     // Add the cut to the subtree under this node only
// )
// {
//     // Create output.
//     Pair<SCIP_Result, SCIP_Cons*> output;
//     auto& [result, cons] = output;
//
//     // Check for duplicate edges.
// #ifdef DEBUG
//     for (Count i = 0; i < cut.nb_edges(); ++i)
//         for (Count j = i + 1; j < cut.nb_edges(); ++j)
//         {
//             const auto [e1_i, e1_j, e1_coeff] = cut.edges(i);
//             const auto [e2_i, e2_j, e2_coeff] = cut.edges(j);
//             release_assert(e1_i != e2_i || e1_j != e2_j,
//                            "Edge ({},{}) is duplicated in robust cut {}",
//                            instance->vertex_name[e1_i],
//                            instance->vertex_name[e1_j],
//                            cut.name());
//         }
// #endif
//
//     // Create a constraint.
// #ifdef MAKE_NAMES
//     const auto name = cut.name().substr(0, 200);
//     const auto name_c_str = name.c_str();
// #else
//     constexpr auto name_c_str = "";
// #endif
//     scip_assert(SCIPcreateConsLinear(scip,
//                                      &cons,
//                                      name_c_str,
//                                      0,
//                                      nullptr,
//                                      nullptr,
//                                      cut.rhs(),
//                                      SCIPinfinity(scip),
//                                      true,
//                                      true,
//                                      true,
//                                      true,
//                                      true,
//                                      static_cast<bool>(node),
//                                      true,
//                                      false,
//                                      false,
//                                      false));
//     debug_assert(cons);
//     cut.set_cons(cons);
//
//     // Print.
// #ifdef MAKE_NAMES
//     debugln("    Creating robust constraint {}:", cut.name());
// #else
//     debugln("    Creating robust constraint:");
// #endif
//
//     // Add variables to the constraint.
// #ifdef DEBUG
//     SCIP_Real lhs = 0.0;
// #endif
//     for (const auto& [var, val, path, path_length] : vars)
//     {
//         // Check.
//         debug_assert(val == SCIPgetSolVal(scip, nullptr, var));
//
//         // Add coefficients.
//         SCIP_Real coeff = 0.0;
//         for (PathLength i = 0; i < path_length - 1; ++i)
//         {
//             const Edge edge{path[i], path[i + 1]};
//             coeff += cut.coeff(edge);
//         }
//
//         // Add coefficient.
//         if (coeff)
//         {
//             // Add coefficient.
//             scip_assert(SCIPaddCoefLinear(scip, cons, var, coeff));
// #ifdef DEBUG
//             lhs += val * coeff;
// #endif
//
//             // Print.
//             debugln("        Val: {:6.4f}, coeff: {:3.0f}, path: {}",
//                     std::abs(val),
//                     coeff,
//                     format_path_with_names(path, path_length, instance->vertex_name));
//         }
//     }
//     debugln("    Given RHS {}, verified RHS {}", cut.rhs(), lhs);
// #ifdef DEBUG
//     debug_assert(SCIPgetStage(scip) == SCIP_STAGE_PROBLEM || SCIPisLT(scip, lhs, cut.rhs()));
// #endif
//
//     // Add the constraint to the problem.
//     if (node)
//     {
//         scip_assert(SCIPaddConsNode(scip, node, cons, nullptr));
//     }
//     else
//     {
//         scip_assert(SCIPaddCons(scip, cons));
//     }
//
//     // Store the cut.
//     robust_cuts.push_back(std::move(cut));
//
//     // Set status.
//     result = SCIP_CONSADDED;
//
//     // Done.
//     return output;
// }

// Add a new robust cut
// Pair<SCIP_Result, SCIP_Row*> Problem::add_robust_cut(
//     SCIP* scip,         // SCIP
//     RobustCut&& cut,    // Data for the cut
//     SCIP_Sepa* sepa     // Separator adding the cut
// )
// {
//     // Create output.
//     Pair<SCIP_Result, SCIP_Row*> output;
//     auto& [result, row] = output;
//
//     // Check for duplicate edges.
// #ifdef DEBUG
//     for (Count i = 0; i < cut.nb_edges(); ++i)
//         for (Count j = i + 1; j < cut.nb_edges(); ++j)
//         {
//             const auto [e1_i, e1_j, e1_coeff] = cut.edges(i);
//             const auto [e2_i, e2_j, e2_coeff] = cut.edges(j);
//             release_assert(e1_i != e2_i || e1_j != e2_j,
//                            "Edge ({},{}) is duplicated in robust cut {}",
//                            instance->vertex_name[e1_i],
//                            instance->vertex_name[e1_j],
//                            cut.name());
//         }
// #endif
//
//     // Create a row.
// #ifdef MAKE_NAMES
//     const auto name = cut.name().substr(0, 200);
//     const auto name_c_str = name.c_str();
// #else
//     constexpr auto name_c_str = "";
// #endif
//     scip_assert(SCIPcreateEmptyRowSepa(scip,
//                                        &row,
//                                        sepa,
//                                        name_c_str,
//                                        cut.rhs(),
//                                        SCIPinfinity(scip),
//                                        false,
//                                        true,
//                                        false));
//     debug_assert(row);
//     cut.set_row(row);
//
//     // Print.
// #ifdef MAKE_NAMES
//     debugln("    Creating robust cut {}:", cut.name());
// #else
//     debugln("    Creating robust cut:");
// #endif
//
//     // Add variables to the row.
// #ifdef DEBUG
//     SCIP_Real lhs = 0.0;
// #endif
//     scip_assert(SCIPcacheRowExtensions(scip, row));
//     for (const auto& [var, val, path, path_length] : vars)
//     {
//         // Check.
//         debug_assert(val == SCIPgetSolVal(scip, nullptr, var));
//
//         // Add coefficients.
//         SCIP_Real coeff = 0.0;
//         for (PathLength idx = 0; idx < path_length - 1; ++idx)
//         {
//             const Edge edge{path[idx], path[idx + 1]};
//             coeff += cut.coeff(edge);
//         }
//
//         // Add coefficient.
//         if (coeff)
//         {
//             // Add coefficient.
//             scip_assert(SCIPaddVarToRow(scip, row, var, coeff));
// #ifdef DEBUG
//             lhs += val * coeff;
// #endif
//
//             // Print.
//             debugln("        Val: {:6.4f}, coeff: {:3.0f}, path: {}",
//                     std::abs(val),
//                     coeff,
//                     format_path_with_names(path, path_length, instance->vertex_name));
//         }
//     }
//     scip_assert(SCIPflushRowExtensions(scip, row));
//     debugln("    Given RHS {}, computed LHS {}", cut.rhs(), lhs);
//     debug_assert(SCIPgetStage(scip) == SCIP_STAGE_PROBLEM || SCIPisLT(scip, lhs, cut.rhs()));
//
//     // Add the row to the LP.
//     SCIP_Bool infeasible;
//     scip_assert(SCIPaddRow(scip, row, true, &infeasible));
//
//     // Store the cut.
//     robust_cuts.push_back(std::move(cut));
//
//     // Set status.
//     result = infeasible ? SCIP_CUTOFF : SCIP_SEPARATED;
//
//     // Done.
//     return output;
// }

// #ifdef USE_SUBSET_ROW_CUTS
// Pair<SCIP_Result, SCIP_Row*> Problem::add_subset_row_cut(
//     SCIP* scip,            // SCIP
//     SubsetRowCut&& cut,    // Data for the cut
//     SCIP_Sepa* sepa        // Separator adding the cut
// )
// {
//     // Create output.
//     Pair<SCIP_Result, SCIP_Row*> output;
//     auto& [result, row] = output;
//
//     // Check for duplicate vertices.
//     debug_assert(cut.i() != cut.j() && cut.i() != cut.k() && cut.j() != cut.k());
//
//     // Check that the cut hasn't been added before. SCIP sometimes removes rows in SCIPlpShrinkRows() so this check is
//     // not valid.
// //#ifdef DEBUG
// //    {
// //        Vector<Vertex> new_cut_vertices{cut.i(), cut.j(), cut.k()};
// //        std::sort(new_cut_vertices.begin(), new_cut_vertices.end());
// //
// //        for (const auto& existing_cut : probdata->subset_row_cuts)
// //        {
// //            Vector<Vertex> existing_cut_vertices{existing_cut.i(), existing_cut.j(), existing_cut.k()};
// //            std::sort(existing_cut_vertices.begin(), existing_cut_vertices.end());
// //
// //            release_assert(new_cut_vertices[0] != existing_cut_vertices[0] ||
// //                           new_cut_vertices[1] != existing_cut_vertices[1] ||
// //                           new_cut_vertices[2] != existing_cut_vertices[2],
// //                           "Adding a subset row cut that already exists");
// //        }
// //    }
// //#endif
//
//     // Create a row.
// #ifdef MAKE_NAMES
//     const auto name = cut.name().substr(0, 200);
//     const auto name_c_str = name.c_str();
// #else
//     constexpr auto name_c_str = "";
// #endif
//     constexpr SCIP_Real rhs = 1;
//     scip_assert(SCIPcreateEmptyRowSepa(scip,
//                                        &row,
//                                        sepa,
//                                        name_c_str,
//                                        -SCIPinfinity(scip),
//                                        rhs,
//                                        false,
//                                        true,
//                                        false));
//     debug_assert(row);
//     cut.set_row(row);
//
//     // Print.
// #ifdef MAKE_NAMES
//     debugln("    Creating subset row cut {}:", cut.name());
// #else
//     debugln("    Creating subset row cut:");
// #endif
//
//     // Add variables to the constraint.
// #ifdef DEBUG
//     SCIP_Real lhs = 0.0;
// #endif
//     scip_assert(SCIPcacheRowExtensions(scip, row));
//     for (const auto& [var, val, path, path_length] : vars)
//     {
//         // Check.
//         debug_assert(val == SCIPgetSolVal(scip, nullptr, var));
//
//         // Add coefficients.
//         SCIP_Real coeff = 0.0;
//         for (PathLength idx = 1; idx < path_length - 1; ++idx)
//         {
//             const auto vertex = path[idx];
//             coeff += (vertex == cut.i()) || (vertex == cut.j()) || (vertex == cut.k());
//         }
//         coeff = std::floor(coeff / 2);
//
//         // Add coefficient.
//         if (coeff)
//         {
//             // Add coefficient.
//             scip_assert(SCIPaddVarToRow(scip, row, var, coeff));
// #ifdef DEBUG
//             lhs += val * coeff;
// #endif
//
//             // Print.
//             debugln("        Val: {:6.4f}, coeff: {:3.0f}, path: {}",
//                     std::abs(val),
//                     coeff,
//                     format_path_with_names(path, path_length, instance->vertex_name));
//         }
//     }
//     scip_assert(SCIPflushRowExtensions(scip, row));
//     debugln("    Given RHS {}, computed LHS {}", cut.rhs(), lhs);
// #ifdef DEBUG
//     debug_assert(SCIPgetStage(scip) == SCIP_STAGE_PROBLEM || SCIPisGT(scip, lhs, rhs));
// #endif
//
//     // Add the row to the LP.
//     SCIP_Bool infeasible;
//     scip_assert(SCIPaddRow(scip, row, true, &infeasible));
//
//     // Store the cut.
//     subset_row_cuts.push_back(std::move(cut));
//
//     // Set status.
//     result = infeasible ? SCIP_CUTOFF : SCIP_SEPARATED;
//
//     // Done.
//     return output;
// }
// #endif

// Update values of edge variables
void Problem::update_variable_values(SCIP* scip)
{
    // Print.
    // debugln("Updating variable values");

    // Update variables values.
    fractional_edges.clear();
    for (auto& [var, val, path] : vars)
    {
        // Get the variable value.
        val = SCIPgetSolVal(scip, nullptr, var);

        // Store the edges of the path.
        if (SCIPisPositive(scip, val) && !SCIPisIntegral(scip, val))
        {
            for (Size idx = 0; idx < path.size() - 1; ++idx)
            {
                const Edge edge{path[idx], path[idx + 1]};
                auto& [edge_val, edge_count] = fractional_edges[edge];
                edge_val += val;
                ++edge_count;
            }
        }
    }

    // Delete edges with integer values.
    for (auto it = fractional_edges.begin(); it != fractional_edges.end();)
    {
        const auto val = it->second.first;
        if (SCIPisIntegral(scip, val))
        {
            it = fractional_edges.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Print.
#ifdef PRINT_DEBUG
    if (!fractional_edges.empty())
    {
        println("    Fractional edges:");
        for (const auto& [edge, pair] : fractional_edges)
        {
            const auto& [val, count] = pair;
            println("        ({},{}): val {:.4f}, count {}", edge.i, edge.j, val, count);
        }
    }
#endif
}

// Write master problem LP relaxation to file
#ifdef DEBUG
void Problem::write_master(SCIP* scip)
{
    static Size iter = 0;
    scip_assert(SCIPwriteLP(scip, fmt::format("master_{}.lp", ++iter).c_str()));
}
#endif
