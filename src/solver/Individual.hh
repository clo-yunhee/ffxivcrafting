#ifndef SOLVER_INDIVIDUAL_HH_
#define SOLVER_INDIVIDUAL_HH_

#include <vector>

#include "../actions/ActionId.hh"
#include "Fitness.hh"

using ActionSequence = std::vector<ActionId>;

struct Individual {
    ActionSequence sequence;
    Fitness        fitness;

    Individual() : sequence{}, fitness{0.0, 0.0, 0.0, 0} {}
    Individual(const ActionSequence& s) : sequence{s}, fitness{0.0, 0.0, 0.0, 0} {}
};

#endif  // SOLVER_INDIVIDUAL_HH_
