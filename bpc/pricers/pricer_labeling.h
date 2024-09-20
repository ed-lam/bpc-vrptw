#pragma once

#include "problem/instance.h"
#include "problem/scip.h"

// Include labeling pricer
SCIP_Retcode SCIPincludePricerLabeling(SCIP* scip, const Instance& instance);
