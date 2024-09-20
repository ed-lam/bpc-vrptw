#include "output/formatting.h"
#include "problem/debug.h"
#include "problem/problem.h"
#include "types/bitset.h"
#include <fmt/ranges.h>

// Make a string of the vertex number in a path
String format_path(
    const Vector<Vertex>& path,   // Path
    const String& separator       // Separator
)
{
    return fmt::format("({})", fmt::join(path, separator));
}

// Make a string of the names of the vertices in a path
String format_path_with_names(
    const Vector<Vertex>& path,           // Path
    const Vector<String>& vertex_name,    // Names of the vertices
    const String& separator                // Separator
)
{
    Size idx = 0;
    auto i = path[idx];
    auto str = fmt::format("({}", vertex_name[i]);
    for (++idx; idx < path.size(); ++idx)
    {
        i = path[idx];
        str += fmt::format("{}{}", separator, vertex_name[i]);
    }
    str += ")";
    return str;
}

// Make a string of 0 and 1 of a bitset
#ifdef DEBUG
String format_bitset(const void* bitset, const Size size)
{
    String str;
    for (Size i = 0; i < size; ++i)
    {
        if (get_bitset(bitset, i))
        {
            str.push_back('1');
        }
        else
        {
            str.push_back('0');
        }
    }
    return str;
}
#endif
