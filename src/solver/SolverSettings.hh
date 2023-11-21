#ifndef SOLVER_SOLVERSETTINGS_HH_
#define SOLVER_SOLVERSETTINGS_HH_

#include <vector>

#include "../actions/ActionId.hh"
#include "../model/Crafter.hh"
#include "../model/Recipe.hh"
#include "../solver/SolverVars.hh"

struct SolverSettings {
    Recipe  recipe;
    Crafter crafter;

    int  maxTrickUses;
    int  reliabilityPercent;
    int  maxLength;
    bool useConditions;

    SolverVars solver;

    std::vector<ActionId> sequence;

    bool debug;
};

#endif  // SOLVER_SOLVERSETTINGS_HH_