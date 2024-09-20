cmake_minimum_required(VERSION 3.21)
project(bpc-vrptw)

# Verbose.
# set(CMAKE_VERBOSE_MAKEFILE on)

# Set C++ version.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)

# Set release build.
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# Set source files to BPC-VRPTW.
set(BPC_VRPTW_SOURCE_FILES
    bpc/branching/constraint_handler_edge_branching.cpp
    bpc/branching/edge_branching.cpp
    bpc/labeling/labeling_algorithm.cpp
    bpc/labeling/memory_pool.cpp
    bpc/labeling/pareto_frontier.cpp
    bpc/main.cpp
    bpc/output/formatting.cpp
    bpc/output/output.cpp
    bpc/pricers/pricer_labeling.cpp
    bpc/problem/instance.cpp
    bpc/problem/problem.cpp
    )

# Create target.
add_executable(bpc-vrptw ${BPC_VRPTW_SOURCE_FILES})
target_include_directories(bpc-vrptw PUBLIC ./ bpc/)

# Include SCIP binaries.
find_path(SCIP_INCLUDE_DIR
          NAMES scip/scip.h
          HINTS "${SCIP_DIR}/scip/src" "$ENV{SCIP_DIR}/scip/src")
find_path(SCIP_CONFIG_DIR
          NAMES scip/config.h
          HINTS "${SCIP_DIR}/build/scip" "$ENV{SCIP_DIR}/build/scip")
find_library(SCIP_LIBRARY
             NAMES scip
             HINTS "${SCIP_DIR}/build/lib" "$ENV{SCIP_DIR}/build/lib")
if (SCIP_INCLUDE_DIR AND SCIP_CONFIG_DIR AND SCIP_LIBRARY)

    target_include_directories(bpc-vrptw PUBLIC ${SCIP_INCLUDE_DIR} ${SCIP_CONFIG_DIR})
    message("Using SCIP binary at ${SCIP_LIBRARY}")

else ()

    # Include Gurobi.
    find_path(GUROBI_INCLUDE_DIR
        NAMES gurobi_c.h
        HINTS "${GUROBI_DIR}/include" "${GUROBI_DIR}/*/include" "$ENV{GUROBI_DIR}/include" "$ENV{GUROBI_DIR}/*/include")
    find_library(GUROBI_LIBRARY
        NAMES gurobi110 gurobi100 gurobi91 gurobi90 gurobi80 gurobi81 gurobi75 gurobi70
        HINTS "${GUROBI_DIR}/lib" "${GUROBI_DIR}/*/lib" "$ENV{GUROBI_DIR}/lib" "$ENV{GUROBI_DIR}/*/lib")
    if (GUROBI_INCLUDE_DIR AND GUROBI_LIBRARY)
        set(LPS grb CACHE STRING "options for LP solver")
        target_include_directories(bpc-vrptw PUBLIC ${GUROBI_INCLUDE_DIR})
        message("Using Gurobi at ${GUROBI_LIBRARY} as LP solver")
    endif ()

    # Include CPLEX.
    if (NOT GUROBI_INCLUDE_DIR OR NOT GUROBI_LIBRARY)
        find_path(CPLEX_INCLUDE_DIR
            NAMES cplex.h
            HINTS "${CPLEX_DIR}/include/ilcplex" "$ENV{CPLEX_DIR}/include/ilcplex")
        find_library(CPLEX_LIBRARY
            NAMES cplex
            HINTS "${CPLEX_DIR}/lib/*/static_pic" "$ENV{CPLEX_DIR}/lib/*/static_pic")
        if (CPLEX_INCLUDE_DIR AND CPLEX_LIBRARY)
            set(LPS cpx CACHE STRING "options for LP solver")
            target_include_directories(bpc-vrptw PUBLIC ${CPLEX_INCLUDE_DIR})
            message("Using CPLEX at ${CPLEX_LIBRARY} as LP solver")
        endif ()
    endif ()

    # Include SoPlex.
    if ((NOT GUROBI_INCLUDE_DIR OR NOT GUROBI_LIBRARY) AND (NOT CPLEX_INCLUDE_DIR OR NOT CPLEX_LIBRARY))
        set(LPS spx CACHE STRING "options for LP solver")
        set(GMP off)
        configure_file(soplex/soplex-config.cmake.in "${CMAKE_BINARY_DIR}/soplex/soplex-config.cmake" @ONLY)
        set(SOPLEX_DIR ${CMAKE_BINARY_DIR}/soplex)
        add_subdirectory(soplex/ EXCLUDE_FROM_ALL)
        message("Using SoPlex as LP solver ${SOPLEX_INCLUDE_DIRS}")
    endif ()

    # Include SCIP.
    set(WITH_SCIPDEF on)
    set(TPI none)
    set(tpisources tpi/tpi_none.c)
    set(THREAD_LIBRARIES "")
    set(TPI_NONE on)
    set(PAPILO off)
    set(READLINE off)
    set(GMP off)
    set(ZLIB off)
    set(ZIMPL off)
    set(IPOPT off)
    configure_file(scip/src/scip/config.h.in "${CMAKE_BINARY_DIR}/scip/config.h" @ONLY)
    target_include_directories(bpc-vrptw PUBLIC ${CMAKE_BINARY_DIR})
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl")
    endif ()
    add_subdirectory(scip/ EXCLUDE_FROM_ALL)
    target_include_directories(bpc-vrptw PRIVATE          scip/src/
                                         SYSTEM INTERFACE scip/src/)
    set(SCIP_LIBRARY libscip)

endif ()

# Include fmt.
add_subdirectory(fmt EXCLUDE_FROM_ALL)
target_include_directories(bpc-vrptw PUBLIC fmt/include/)

# Include cxxopts.
target_include_directories(bpc-vrptw PUBLIC cxxopts/include/)

# Include unordered_dense.
target_include_directories(bpc-vrptw PUBLIC unordered_dense/include/)

# Link to math library.
# find_library(LIBM m)
# if (NOT LIBM)
#     set(LIBM "")
# endif ()

# Link to libraries.
target_link_libraries(bpc-vrptw fmt::fmt-header-only ${SCIP_LIBRARY} ${LIBM})

# Set general options.
# target_compile_options(bpc-vrptw PRIVATE -DRANDOM_SEED=1234)

# Set pricing options.
# target_compile_options(bpc-vrptw PRIVATE -DUSE_HEURISTIC_PRICING)
# target_compile_options(bpc-vrptw PRIVATE -DUSE_NG_ROUTE_PRICING)
# target_compile_options(bpc-vrptw PRIVATE -DNG_ROUTE_NEIGHBOURHOOD_SIZE=10)
# target_compile_options(bpc-vrptw PRIVATE -DHEURISTIC_PRICER_MIN_NB_EDGES_1=3)
# target_compile_options(bpc-vrptw PRIVATE -DHEURISTIC_PRICER_MIN_NB_EDGES_2=5)

# Set separator options.
# target_compile_options(bpc-vrptw PRIVATE -DUSE_ROUNDED_CAPACITY_CUTS)
# target_compile_options(bpc-vrptw PRIVATE -DUSE_TWO_PATH_CUTS)
# target_compile_options(bpc-vrptw PRIVATE -DUSE_SUBSET_ROW_CUTS)
# target_compile_options(bpc-vrptw PRIVATE -DROUNDED_CAPACITY_CUTS_MAX_DEPTH=6)
# target_compile_options(bpc-vrptw PRIVATE -DTWO_PATH_CUTS_MAX_DEPTH=6)
# target_compile_options(bpc-vrptw PRIVATE -DSUBSET_ROW_CUTS_MAX_DEPTH=6)
# target_compile_options(bpc-vrptw PRIVATE -DTWO_PATH_CUTS_MIN_SUBSET_SIZE=3)
# target_compile_options(bpc-vrptw PRIVATE -DTWO_PATH_CUTS_MAX_SUBSET_SIZE=16)
# target_compile_options(bpc-vrptw PRIVATE -DTWO_PATH_HEURISTIC_SKIP=4)
# target_compile_options(bpc-vrptw PRIVATE -DTWO_PATH_ITERATION_LIMIT=10000)

# Set branching rule options.
target_compile_options(bpc-vrptw PRIVATE -DUSE_BRANCH_ON_VEHICLES)
target_compile_options(bpc-vrptw PRIVATE -DSTRONG_BRANCHING_NUM_EDGES_FOR_STRONG_BRANCHING=30)
target_compile_options(bpc-vrptw PRIVATE -DSTRONG_BRANCHING_INFEASIBLE_LB_VALUE=1000.0)

# Set node selection options.
target_compile_options(bpc-vrptw PRIVATE -DUSE_BEST_FIRST_NODE_SELECTION)
# target_compile_options(bpc-vrptw PRIVATE -DUSE_DEPTH_FIRST_NODE_SELECTION)
# target_compile_options(bpc-vrptw PRIVATE -DUSE_BEST_ESTIMATE_NODE_SELECTION)
# target_compile_options(bpc-vrptw PRIVATE -DUSE_HYBRID_ESTIMATE_BOUND_NODE_SELECTION)
# target_compile_options(bpc-vrptw PRIVATE -DUSE_RESTART_DEPTH_FIRST_NODE_SELECTION)

# Set warnings.
# Set warnings.
target_compile_options(bpc-vrptw PRIVATE
                       -Wall
                       -Wextra
                       -Wignored-qualifiers
                       -Werror=return-type
                       -Wno-sign-compare
                       -Wno-narrowing)
# set_source_files_properties(cvrpsep/grsearch.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-but-set-variable)
# set_source_files_properties(cvrpsep/mxf.cpp PROPERTIES COMPILE_FLAGS -Wno-format-extra-args)

# Set flags.
target_compile_options(bpc-vrptw PRIVATE -march=native)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(bpc-vrptw PRIVATE -DDEBUG -D_GLIBCXX_DEBUG)
    message("Compiled in debug mode")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(bpc-vrptw PRIVATE -Og -DNDEBUG)
    message("Compiled in release with debug info mode")
else ()
    target_compile_options(bpc-vrptw PRIVATE -O3 -funroll-loops -DNDEBUG)
    message("Compiled in release mode")
endif ()

# Use address sanitizer.
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

# Turn on link-time optimization for Linux.
# if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#     set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
# endif ()
