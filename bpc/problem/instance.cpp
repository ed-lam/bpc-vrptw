// #define PRINT_DEBUG

#include "problem/instance.h"
#include "types/bitset.h"
#include <cmath>
#include <fstream>
#include <regex>

struct VertexData
{
    String name;
    Position x;
    Position y;
    Load load;
    Time earliest;
    Time latest;
    Time service;
};

// static void create_unreachable_customers(Instance& instance)
// {
//     // Get instance data.
//     const auto num_customers = instance.num_customers();
//     const auto depot = instance.depot();
//     const auto& vertex_load = instance.vertex_load;
//     const auto& vertex_earliest = instance.vertex_earliest;
//     const auto& vertex_latest = instance.vertex_latest;
//     const auto& service_plus_travel = instance.service_plus_travel;
//     const auto& vehicle_load_capacity = instance.vehicle_load_capacity;
//     auto& vertex_unreachable = instance.vertex_unreachable;
//
//     // Calculate the number of bytes to store the unreachable customers.
//     const auto unreachable_size = (num_customers + CHAR_BIT - 1) / CHAR_BIT;
//     vertex_unreachable.resize(unreachable_size * num_customers, 0);
//
//     // Select the customers that cannot be visited after visiting a customer.
//     for (Vertex i = 0; i < num_customers; ++i)
//     {
//         // Get the unreachable customers of i.
//         auto i_unreachable = &vertex_unreachable[i * unreachable_size];
//
//         // Customer i itself is unreachable.
//         set_bitset(i_unreachable, i);
//
//         // Customer j is unreachable if exceeding the load, time and energy constraints.
//         for (Vertex j = 0; j < num_customers; ++j)
//             if ((vertex_load[i] + vertex_load[j] > vehicle_load_capacity) ||
//                 (vertex_earliest[i] + service_plus_travel(i, j) > vertex_latest[j]) ||
//                 (vertex_earliest[i] + service_plus_travel(i, j) + service_plus_travel(j, depot) > vertex_latest[depot]))
//             {
//                 set_bitset(i_unreachable, j);
//             }
//     }
// }

// #ifdef USE_NG_ROUTE_PRICING
// static void create_ng_route_neighbourhood(Instance& instance)
// {
//     // Get instance data.
//     const auto num_customers = instance.num_customers();
//     const auto& vertex_unreachable = instance.vertex_unreachable;
//     const auto& cost = instance.cost;
//     auto& vertex_ng_route_neighbourhood = instance.vertex_ng_route_neighbourhood;
//
//     // Calculate the number of bytes to store the unreachable customers.
//     const auto unreachable_size = (num_customers + CHAR_BIT - 1) / CHAR_BIT;
//     vertex_ng_route_neighbourhood.resize(unreachable_size * num_customers, 0);
//
//     // Select the customers that cannot be visited after visiting a customer.
//     for (Vertex i = 0; i < num_customers; ++i)
//     {
//         // Create candidates of reachable customers for the neighbourhood.
//         const auto i_unreachable = &vertex_unreachable[i * unreachable_size];
//         Vector<VertexVal> cost_candidates;
//         for (Vertex j = 0; j < num_customers; ++j)
//             if (!get_bitset(i_unreachable, j))
//             {
//                 debug_assert(i != j);
//                 cost_candidates.push_back({j, cost(i, j)});
//             }
//         std::sort(cost_candidates.begin(),
//                   cost_candidates.end(),
//                   [](const VertexVal a, const VertexVal b) { return a.val < b.val; });
//
//         // Select the first few customers, ordered by lowest distance.
//         auto i_ng_route_neighbourhood = &vertex_ng_route_neighbourhood[i * unreachable_size];
//         // Count count = 0;
//         for (size_t size = std::min<size_t>(cost_candidates.size(), NG_ROUTE_NEIGHBOURHOOD_SIZE), idx = 0;
//              idx < size;
//              ++idx)
//         // for (size_t idx = 0; idx < cost_candidates.size(); ++idx)
//         {
//             release_assert(!get_bitset(i_ng_route_neighbourhood, cost_candidates[idx].vertex)); // TODO
//             // if (!get_bitset(i_ng_route_neighbourhood, cost_candidates[idx].vertex))
//             // {
//                 set_bitset(i_ng_route_neighbourhood, cost_candidates[idx].vertex);
//                 // ++count;
//                 // if (count == NG_ROUTE_NEIGHBOURHOOD_SIZE)
//                 // {
//                 //     break;
//                 // }
//             // }
//         }
//
//         // Print.
// #ifdef PRINT_DEBUG
//         println("    ng-route neighbourhood of customer {}:", instance.vertex_name[i]);
//         for (Vertex j = 0; j < num_customers; ++j)
//             if (get_bitset(i_ng_route_neighbourhood, j))
//             {
//                 println("        {}", instance.vertex_name[j]);
//             }
// #endif
//     }
// }
// #endif

Instance::Instance(const std::filesystem::path& instance_path)
{
    // Get instance name.
    name = instance_path.filename().stem().string();

    // Open instance file.
    std::ifstream file;
    file.open(instance_path, std::ios::in);
    release_assert(file.good(), "Cannot find instance file {}", instance_path.string());
    String line;

    // Read the vehicle data.
    vehicle_load_capacity = -1;
    while (std::getline(file, line))
    {
        std::smatch match;
        if (std::regex_match(line, match, std::regex(R"(Q:\s*(\d+\.*\d*))")))
        {
            vehicle_load_capacity = std::ceil(std::atof(match[1].str().c_str()));
            break;
        }
    }
    release_assert(vehicle_load_capacity >= 0, "Vehicle load capacity not found");

    // Read the vertex data.
    Vector<VertexData> vertices;
    {
        release_assert(std::getline(file, line), "Header row is not correct");
        release_assert(std::getline(file, line), "Header row is not correct");

        std::smatch match;
        release_assert(std::regex_match(line, match, std::regex(R"(\s*(\w+)\s*(\w+)\s*(\w+)\s*(\w+)\s*(\w+)\s*(\w+)\s*(\w+)\s*)")),
                       "Header row is not correct");
        release_assert(match.size() == 8, "Header row is not correct");

        const auto tmp_vertex_name = match[1].str();
        const auto tmp_vertex_x = match[2].str();
        const auto tmp_vertex_y = match[3].str();
        const auto tmp_vertex_load = match[4].str();
        const auto tmp_vertex_earliest = match[5].str();
        const auto tmp_vertex_latest = match[6].str();
        const auto tmp_vertex_service = match[7].str();

        release_assert(tmp_vertex_name == "R", "Header row is not correct");
        release_assert(tmp_vertex_x == "X", "Header row is not correct");
        release_assert(tmp_vertex_y == "Y", "Header row is not correct");
        release_assert(tmp_vertex_load == "Q", "Header row is not correct");
        release_assert(tmp_vertex_earliest == "A", "Header row is not correct");
        release_assert(tmp_vertex_latest == "B", "Header row is not correct");
        release_assert(tmp_vertex_service == "S", "Header row is not correct");
    }
    Time time_horizon = -1;
    while (std::getline(file, line))
    {
        std::smatch match;
        if (std::regex_match(line, match, std::regex(R"(\s*([^\s\\]+)\s+(-*\d+\.*\d*)\s+(-*\d+\.*\d*)\s+(\d+\.*\d*)\s+(\d+\.*\d*)\s+(\d+\.*\d*)\s+(\d+\.*\d*)\s*)")))
        {
            release_assert(match.size() == 8, "Vertex row is not correct");

            const auto tmp_vertex_name = match[1].str();
            const auto tmp_vertex_x = std::round(std::atof(match[2].str().c_str()));
            const auto tmp_vertex_y = std::round(std::atof(match[3].str().c_str()));
            const auto tmp_vertex_load = std::round(std::atof(match[4].str().c_str()));
            const auto tmp_vertex_earliest = std::round(std::atof(match[5].str().c_str()));
            const auto tmp_vertex_latest = std::round(std::atof(match[6].str().c_str()));
            const auto tmp_vertex_service = std::round(std::atof(match[7].str().c_str()));

            release_assert(tmp_vertex_load >= 0, "Vertex load is negative");
            release_assert(tmp_vertex_earliest >= 0, "Vertex earliest start time is negative");
            release_assert(tmp_vertex_latest >= tmp_vertex_earliest,
                           "Vertex latest start time is before earliest start time");
            release_assert(tmp_vertex_service >= 0, "Vertex service duration is negative");

            if (vertices.empty())
            {
                time_horizon = std::ceil(tmp_vertex_latest);
            }

            vertices.push_back({std::move(tmp_vertex_name),
                                static_cast<Position>(std::ceil(tmp_vertex_x)),
                                static_cast<Position>(std::ceil(tmp_vertex_y)),
                                static_cast<Load>(std::ceil(tmp_vertex_load)),
                                static_cast<Time>(std::ceil(tmp_vertex_earliest)),
                                static_cast<Time>(std::ceil(tmp_vertex_latest)),
                                static_cast<Time>(std::ceil(tmp_vertex_service))});
        }
        else
        {
            break;
        }
    }

    // Check.
    release_assert(time_horizon != -1, "There is no depot vertex");
    release_assert(!vertices.empty(), "No vertices found");
    {
        const auto& depot = vertices[0];
        release_assert(depot.load == 0,
                       "Depot {} must have 0 load", depot.name);
        release_assert(depot.earliest == 0 && depot.latest == time_horizon,
                       "Depot {} must have a time window from 0 to the time horizon {}", depot.name, time_horizon);
        release_assert(depot.service == 0,
                       "Depot {} must have 0 service duration", depot.name);
    }
    for (Vertex i = 0; i < static_cast<Vertex>(vertices.size()); ++i)
        for (Vertex j = i + 1; j < static_cast<Vertex>(vertices.size()); ++j)
        {
            release_assert(vertices[i].name != vertices[j].name, "Vertex name {} is not unique", vertices[i].name);
        }

    // Close file.
    file.close();

    // Move the depot vertex to the end.
    vertices.push_back(vertices.front());
    vertices.erase(vertices.begin());

    // Store the vertices.
    vertex_name.resize(vertices.size());
    vertex_x.resize(vertices.size());
    vertex_y.resize(vertices.size());
    vertex_load.resize(vertices.size());
    vertex_earliest.resize(vertices.size());
    vertex_latest.resize(vertices.size());
    vertex_service.resize(vertices.size());
    for (Vertex i = 0; i < num_vertices(); ++i)
    {
        vertex_name[i] = std::move(vertices[i].name);
        vertex_x[i] = vertices[i].x;
        vertex_y[i] = vertices[i].y;
        vertex_load[i] = vertices[i].load;
        vertex_earliest[i] = vertices[i].earliest;
        vertex_latest[i] = vertices[i].latest;
        vertex_service[i] = vertices[i].service;
    }

    // Get NaN.
    debug_assert(std::numeric_limits<Cost>::has_quiet_NaN);
    constexpr auto nan = std::numeric_limits<Cost>::quiet_NaN();

    // Calculate the resource matrices.
    Matrix<Cost> all_cost(num_vertices(), num_vertices(), nan);
    service_plus_travel.clear_and_resize(num_vertices(), num_vertices());
    for (Vertex i = 0; i < num_vertices(); ++i)
        for (Vertex j = 0; j < num_vertices(); ++j)
        {
            const auto dx = vertex_x[i] - vertex_x[j];
            const auto dy = vertex_y[i] - vertex_y[j];

            all_cost(i, j) = std::ceil(std::sqrt(dx * dx + dy * dy));
            service_plus_travel(i, j) = std::ceil(all_cost(i, j) + vertex_service[i]);
        }
#ifdef DEBUG
    auto debug_cost = all_cost;
#endif

    // Tighten the time windows.
    for (Vertex i = 0; i < num_customers(); ++i)
    {
        {
            const auto h = depot();
            vertex_earliest[i] = std::max<Time>(vertex_earliest[i], vertex_earliest[h] + service_plus_travel(h, i));
        }
        {
            const auto j = depot();
            vertex_latest[i] = std::min<Time>(vertex_latest[i], vertex_latest[j] - service_plus_travel(i, j));
        }
        release_assert(vertex_earliest[i] <= vertex_latest[i],
                       "Customer {} has time window tightened to empty time window [{},{}]",
                       vertex_name[i], vertex_earliest[i], vertex_latest[i]);
    }

    // Create the edges.
    cost = Matrix<Cost>(num_vertices(), num_vertices(), nan);
    {
        // Depot to customers.
        const auto i = depot();
        for (Vertex j = 0; j < num_customers(); ++j)
        {
            cost(i, j) = all_cost(i, j);
        }
    }
    for (Vertex i = 0; i < num_customers(); ++i)
    {
        // Customers to customers.
        for (Vertex j = 0; j < num_customers(); ++j)
        {
            cost(i, j) = all_cost(i, j);
        }

        // Customers to depot.
        {
            const auto j = depot();
            cost(i, j) = all_cost(i, j);
        }
    }
    // Remove infeasible edges.
    for (Vertex i = 0; i < num_vertices(); ++i)
        for (Vertex j = 0; j < num_vertices(); ++j)
            if ((i == j) ||                                                                                   // Loops
                (vertex_load[i] + vertex_load[j] > vehicle_load_capacity) ||                                  // Edge exceeds vehicle load capacity
                (vertex_earliest[i] + service_plus_travel(i, j) > vertex_latest[j]))                          // Earliest departure exceeds time window
            {
                cost(i, j) = nan;
            }

    // Calculate the customers unreachable when extending outwards.
    // create_unreachable_customers(*this);

    // Calculate the neighbourhood of each vertex for ng-route pricing.
// #ifdef USE_NG_ROUTE_PRICING
//     create_ng_route_neighbourhood(*this);
// #endif

    // Print.
#ifdef PRINT_DEBUG
    debug_separator();
    cost.print(vertex_name.data(), vertex_name.data());
    service_plus_travel.print(vertex_name.data(), vertex_name.data());
#endif
}
