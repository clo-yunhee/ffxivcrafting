#ifndef ACTIONS_ACTION_HH_
#define ACTIONS_ACTION_HH_

#include <initializer_list>
#include <vector>

#include "ActionId.hh"
#include "ActionType.hh"

struct Action {
    Action(ActionId id, const char *shortName, const char *fullName, int durabilityCost,
           int cpCost, double successProbability, double qualityIncreaseMultiplier,
           double progressIncreaseMultiplier, ActionType type, int activeTurns,
           int level);

    Action(ActionId id, const char *shortName, const char *fullName, int durabilityCost,
           int cpCost, double successProbability, double qualityIncreaseMultiplier,
           double progressIncreaseMultiplier, ActionType type, int activeTurns, int level,
           bool onGood, bool onExcellent, bool onPoor);

    Action(ActionId id, const char *shortName, const char *fullName, int durabilityCost,
           int cpCost, double successProbability, double qualityIncreaseMultiplier,
           double progressIncreaseMultiplier, ActionType type, int activeTurns, int level,
           std::initializer_list<ActionId> comboActions);

    const ActionId              id;
    const char                 *shortName;
    const char                 *fullName;
    const int                   durabilityCost;
    const int                   cpCost;
    const double                successProbability;
    const double                qualityIncreaseMultiplier;
    const double                progressIncreaseMultiplier;
    const ActionType            type;
    const int                   activeTurns;
    const int                   level;
    const bool                  isConditional;
    const bool                  onGood;
    const bool                  onExcellent;
    const bool                  onPoor;
    const bool                  isCombo;
    const std::vector<ActionId> comboActions;
};

#endif  // ACTIONS_ACTION_HH_
