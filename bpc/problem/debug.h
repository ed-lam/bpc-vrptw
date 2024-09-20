#pragma once

#include <fmt/format.h>

#ifdef DEBUG
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x)
#endif

#ifdef DEBUG
#define DEBUG_PARAM(x) , x
#else
#define DEBUG_PARAM(x)
#endif

#ifdef DEBUG
#define println(format, ...) do { \
    fmt::print(format "\n", ##__VA_ARGS__); \
    fflush(stdout); \
} while (false)
#else
#define println(format, ...) do { \
    fmt::print(format "\n", ##__VA_ARGS__); \
} while (false)
#endif

#ifdef PRINT_DEBUG
#define debugln(format, ...) println(format, ##__VA_ARGS__)
#else
#define debugln(format, ...) {}
#endif

#define debug_separator(...) debugln("====================================================================================================")
#define print_separator(...) println("====================================================================================================")

#ifdef DEBUG
#define err(format, ...) do {                                  \
    fmt::print(stderr, "Error: " format "\n", ##__VA_ARGS__);  \
    fmt::print(stderr, "Function: {}\n", __PRETTY_FUNCTION__); \
    fmt::print(stderr, "File: {}\n", __FILE__);                \
    fmt::print(stderr, "Line: {}\n", __LINE__);                \
    fflush(stderr);                                            \
    std::abort();                                              \
} while (false)
#else
#define err(format, ...) do {                                 \
    fmt::print(stderr, "Error: " format "\n", ##__VA_ARGS__); \
    std::abort();                                             \
} while (false)
#endif

#define release_assert(condition, ...) do { \
    if (!(condition)) { err(__VA_ARGS__); }     \
} while (false)

#ifdef DEBUG
#define debug_assert(condition) release_assert(condition, "{}", #condition)
#else
#define debug_assert(condition) {}
#endif

#define scip_assert(statement, ...) do {                \
    const int error = statement;                       \
    release_assert(error == 1, "SCIP error {}", error); \
} while (false)

#define NOT_YET_IMPLEMENTED() do { \
    err("Reached unimplemented section in function \"{}\"", __PRETTY_FUNCTION__); \
} while (false)

#define NOT_YET_CHECKED() do { \
    err("Reached unchecked section in function \"{}\"", __PRETTY_FUNCTION__); \
} while (false)

#define UNREACHABLE() do { \
    err("Reached unreachable section in function \"{}\"", __PRETTY_FUNCTION__); \
} while (false)
