// #include "problem/instance.h"
// #include "problem/solution.h"
#include "output/formatting.h"
#include "output/output.h"
#include "problem/debug.h"
#include "problem/problem.h"
#include "problem/scip.h"
#include <fmt/color.h>

// Print paths with positive value
void print_positive_paths(
    SCIP* scip,      // SCIP
    SCIP_Sol* sol    // Solution
)
{
    // Get problem.
    auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
    const auto& vars = problem.vars;
    const auto& instance = *problem.instance;
    const auto num_customers = instance.num_customers();

    // Get fractional edges and determine if the solution is fractional.
    Bool is_fractional = false;
    HashMap<Edge, SCIP_Real> fractional_edges;
    for (const auto& [var, _, path] : vars)
    {
        // Get the variable value.
        const auto val = SCIPgetSolVal(scip, sol, var);

        // Store the vertices and edges of the path.
        if (SCIPisPositive(scip, val) && !SCIPisIntegral(scip, val))
        {
            for (Size idx = 0; idx < path.size() - 1; ++idx)
            {
                const Edge edge{path[idx], path[idx + 1]};
                fractional_edges[edge] += val;
            }
        }

        // Store fractional status.
        is_fractional |= !SCIPisIntegral(scip, val);
    }

    // Delete edges with integer values.
    for (auto it = fractional_edges.begin(); it != fractional_edges.end();)
    {
        const auto& [edge, val] = *it;
        if (SCIPisIntegral(scip, val))
        {
            it = fractional_edges.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Print header.
    if (sol)
    {
        println("Paths from solution with objective value {:.4f}", SCIPgetSolOrigObj(scip, sol));
    }
    else if (SCIPgetStage(scip) == SCIP_STAGE_SOLVING)
    {
        println("    Paths at node {}, depth {}, obj {:.4f}",
                SCIPnodeGetNumber(SCIPgetCurrentNode(scip)),
                SCIPgetDepth(scip),
                SCIPgetLPObjval(scip));
    }
    else
    {
        UNREACHABLE();
    }

    // Print paths.
    for (const auto& [var, _, path] : vars)
    {
        // Get the variable value.
        const auto val = SCIPgetSolVal(scip, sol, var);

        // Print.
        if ((is_fractional && SCIPisPositive(scip, val) && !SCIPisIntegral(scip, val)) ||
            (!is_fractional && SCIPisPositive(scip, val)))
        {
            Cost path_cost = 0;
            for (Size idx = 0; idx < path.size() - 1; ++idx)
            {
                path_cost += instance.cost(path[idx], path[idx + 1]);
            }

            if (SCIPgetStage(scip) == SCIP_STAGE_SOLVING)
            {
                fmt::print("    ");
            }
            if (SCIPisIntegral(scip, val))
            {
                fmt::print("    ");
            }
            else
            {
                fmt::print("  --");
            }
            fmt::print("Val: {:6.4f}, cost: {:5.0f}, path: ", std::abs(val), path_cost);

            bool prev_edge_is_fractional = false;
            Size idx = 0;
            for (; idx < path.size() - 1; ++idx)
            {
                const auto i = path[idx];
                const auto j = path[idx + 1];

                const Edge edge{i, j};
                const auto edge_is_fractional = (fractional_edges.find(edge) != fractional_edges.end());

                const auto str = fmt::format("{} ({})", i, instance.vertex_name[i]);
                if (edge_is_fractional || prev_edge_is_fractional)
                {
                    if (i < num_customers)
                    {
                        fmt::print(fmt::emphasis::bold | fg(fmt::terminal_color::green), "{:>16s}", str);
                    }
                    else
                    {
                        fmt::print(fmt::emphasis::bold, "{:>16s}", str);
                    }
                }
                else
                {
                    if (i < num_customers)
                    {
                        fmt::print(fg(fmt::terminal_color::green), "{:>16s}", str);
                    }
                    else
                    {
                        fmt::print("{:>16s}", str);
                    }
                }
                prev_edge_is_fractional = edge_is_fractional;
            }

            {
                const auto i = path[idx];

                const auto str = fmt::format("{} ({})", i, instance.vertex_name[i]);
                if (prev_edge_is_fractional)
                {
                    if (i < num_customers)
                    {
                        fmt::print(fmt::emphasis::bold | fg(fmt::terminal_color::green), "{:>16s}", str);
                    }
                    else
                    {
                        fmt::print(fmt::emphasis::bold, "{:>16s}", str);
                    }
                }
                else
                {
                    if (i < num_customers)
                    {
                        fmt::print(fg(fmt::terminal_color::green), "{:>16s}", str);
                    }
                    else
                    {
                        fmt::print("{:>16s}", str);
                    }
                }
            }

            println("");
        }
    }
}

void print_best_solution(SCIP* scip)
{
    auto sol = SCIPgetBestSol(scip);
    if (sol)
    {
        println("");
        print_separator();
        println("");
        print_positive_paths(scip, sol);
#ifndef DEBUG
        println("");
        print_solution(scip, sol);
#endif
    }
}

// Print solution
void print_solution(SCIP* scip, SCIP_Sol* sol)
{
    // Get problem.
    auto& problem = *reinterpret_cast<Problem*>(SCIPgetProbData(scip));
    const auto& vars = problem.vars;
    const auto& instance = *problem.instance;
    const auto& depot = instance.depot();
    const auto& vertex_name = instance.vertex_name;
    const auto& vertex_load = instance.vertex_load;
    const auto& vertex_earliest = instance.vertex_earliest;
    const auto& cost = instance.cost;
    const auto& service_plus_travel = instance.service_plus_travel;

    // Print solution.
    println("Objective value: {}", SCIPgetSolOrigObj(scip, sol));
    println("-----------------------------------------------------------------");
    println("{:>13s}{:>13s}{:>13s}{:>13s}{:>13s}", "Vehicle", "Vertex", "Cost", "Load", "Time");
    println("-----------------------------------------------------------------");
    Size vehicle = 0;
    for (const auto& [var, _, path] : vars)
    {
        const auto val = SCIPgetSolVal(scip, sol, var);
        if (SCIPisEQ(scip, val, 1.0))
        {
            Vertex h = depot;
            Cost route_cost = 0;
            Load route_load = 0;
            Time route_time = 0;
            for (Size idx = 0; idx < path.size(); ++idx)
            {
                const auto i = path[idx];
                route_cost = route_cost + (idx != 0 ? cost(h, i) : 0);
                route_load += vertex_load[i];
                route_time = std::max<Time>(route_time + service_plus_travel(h, i), vertex_earliest[i]);
                println("{:>13d}{:>13s}{:>13.2f}{:>13d}{:>13d}", vehicle, vertex_name[i], route_cost, route_load, route_time);
                h = i;
            }
            println("-----------------------------------------------------------------");
            ++vehicle;
        }
    }
}
