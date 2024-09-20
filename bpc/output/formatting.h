#pragma once

#include "problem/scip.h"
#include "types/basic_types.h"
#include "types/string.h"
#include "types/vector.h"

// Make a string of the vertex number in a path
String format_path(
    const Vector<Vertex>& path,       // Path
    const String& separator = ", "    // Separator
);

// Make a string of the names of the vertices in a path
String format_path_with_names(
    const Vector<Vertex>& path,           // Path
    const Vector<String>& vertex_name,    // Names of the vertices
    const String& separator = ", "        // Separator
);

// // Make a string of the numbers and names of the vertices in a path
// #ifdef DEBUG
// String format_path_vertex_numbers_and_names(
//     const Vertex* const path,             // Path
//     const Count path_length,              // Path length
//     const Vector<String>& vertex_name,    // Names of the vertices
//     const String separator = ", "         // Separator
// );
// #endif

// Make a string of 0 and 1 of a bitset
#ifdef DEBUG
String format_bitset(const void* bitset, const Size size);
#endif
