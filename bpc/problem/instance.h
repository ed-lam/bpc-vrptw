#pragma once

#include "problem/debug.h"
#include "types/basic_types.h"
#include "types/matrix.h"
#include "types/string.h"
#include "types/vector.h"
#include <filesystem>

// #define IS_CUSTOMER(i)                    (i < num_customers)
// #define IS_DEPOT(i)                       (i == num_customers)
//
// #define CUSTOMER_BEGIN                    (0)
// #define CUSTOMER_END                      (num_customers)
// #define DEPOT                             (num_customers)
// #define VERTICES_BEGIN                    (0)
// #define VERTICES_END                      (num_customers + 1)

struct Instance
{
    // Instance
    String name;

    // Vertices data
    Vector<String> vertex_name;
    Vector<Position> vertex_x;
    Vector<Position> vertex_y;
    Vector<Load> vertex_load;
    Vector<Time> vertex_earliest;
    Vector<Time> vertex_latest;
    Vector<Time> vertex_service;
    Vector<Byte> vertex_unreachable;
#ifdef USE_NG_ROUTE_PRICING
    Vector<Byte> vertex_ng_route_neighbourhood;
#endif
    Matrix<Cost> cost;
    Matrix<Time> service_plus_travel;

    // Vehicles data
    Load vehicle_load_capacity;

  public:
    // Constructors and destructor
    Instance(const std::filesystem::path& instance_path);
    Instance() = delete;
    Instance(const Instance&) = delete;
    Instance(Instance&&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance& operator=(Instance&&) = delete;
    ~Instance() = default;

    // Getters
    inline Vertex depot() const { return num_customers(); }
    inline Vertex num_customers() const { return num_vertices() - 1; }
    inline Vertex num_vertices() const { return vertex_load.size(); }
    inline auto is_valid(const Vertex i, const Vertex j) const { return !std::isnan(cost(i, j)); }
};
