#ifndef MODEL_SYNTH_HH_
#define MODEL_SYNTH_HH_

#include <vector>

#include "../actions/ActionId.hh"

class Crafter;
class Recipe;
class SolverVars;

struct Synth {
    Synth(const Crafter &crafter, const Recipe &recipe, int maxTrickUses,
          int reliabilityIndex, bool useConditions, int maxLength,
          const SolverVars &solverVars);

    double calculateBaseProgressIncrease(int effCrafterLevel, int craftsmanship) const;
    double calculateBaseQualityIncrease(int effCrafterLevel, int control) const;

    double probabilityOfGood() const;
    double probabilityOfExcellent() const;

    int getEffectiveCrafterLevel() const;

    std::vector<ActionId> buildHeuristicSequence() const;

    const Crafter    &crafter;
    const Recipe     &recipe;
    const SolverVars &solverVars;
    const int         maxTrickUses;
    const int         reliabilityIndex;
    const bool        useConditions;
    const int         maxLength;
};

#endif  // MODEL_SYNTH_HH_