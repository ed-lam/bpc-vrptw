#pragma once

#include "problem/scip.h"

// Print paths with positive value
void print_positive_paths(
    SCIP* scip,                // SCIP
    SCIP_Sol* sol = nullptr    // Solution
);

// Print best solution
void print_best_solution(SCIP* scip);

// // Print solution
void print_solution(SCIP* scip, SCIP_Sol* sol);
