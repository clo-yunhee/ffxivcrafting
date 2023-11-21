#ifndef MODEL_CRAFTER_HH_
#define MODEL_CRAFTER_HH_

#include <vector>

#include "../actions/ActionId.hh"
#include "CrafterClass.hh"

struct Crafter {
    bool hasAction(ActionId action) const;

    CrafterClass          cls;
    int                   level;
    int                   craftsmanship;
    int                   control;
    int                   craftingPoints;
    bool                  isSpecialist;
    std::vector<ActionId> actions;
};

#endif  // MODEL_CRAFTER_HH_