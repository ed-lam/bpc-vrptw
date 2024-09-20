#pragma once

#include "types/basic_types.h"
#include "types/hash_map.h"
#include <type_traits>

// struct EdgeVal
// {
//     Vertex i;
//     Vertex j;
//     Float val;
// };
// static_assert(std::is_trivial<EdgeVal>::value);

struct Edge
{
    Vertex i;
    Vertex j;

    // Constructors and destructor
    Edge(const Vertex i, const Vertex j) : i(i), j(j) {}
    Edge() = default;
    Edge(const Edge&) = default;
    Edge(Edge&&) = default;
    Edge& operator=(const Edge&) = default;
    Edge& operator=(Edge&&) = default;
    ~Edge() = default;

    // Get ID of the edge
    uint32_t id() const { return *reinterpret_cast<const uint32_t*>(this); }
};
static_assert(sizeof(Edge) == sizeof(uint32_t));
static_assert(std::is_trivial<Edge>::value);
inline bool operator==(const Edge a, const Edge b)
{
    return a.id() == b.id();
}
inline bool operator!=(const Edge a, const Edge b)
{
    return !(a == b);
}

template<>
struct ankerl::unordered_dense::hash<Edge> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const Edge& e) const noexcept -> uint64_t {
        static_assert(std::has_unique_object_representations_v<Edge>);
        return ankerl::unordered_dense::detail::wyhash::hash(&e, sizeof(e));
    }
};