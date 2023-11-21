#include "Crafter.hh"

#include <algorithm>

bool Crafter::hasAction(ActionId action) const {
    return std::find(actions.begin(), actions.end(), action) != actions.end();
}