#include "Action.hh"

Action::Action(ActionId id, const char *shortName, const char *fullName,
               int durabilityCost, int cpCost, double successProbability,
               double qualityIncreaseMultiplier, double progressIncreaseMultiplier,
               ActionType type, int activeTurns, int level)
    : id(id),
      shortName(shortName),
      fullName(fullName),
      durabilityCost(durabilityCost),
      cpCost(cpCost),
      successProbability(successProbability),
      qualityIncreaseMultiplier(qualityIncreaseMultiplier),
      progressIncreaseMultiplier(progressIncreaseMultiplier),
      type(type),
      activeTurns(activeTurns),
      level(level),
      isConditional(false),
      onGood(false),
      onExcellent(false),
      onPoor(false),
      isCombo(false) {}

Action::Action(ActionId id, const char *shortName, const char *fullName,
               int durabilityCost, int cpCost, double successProbability,
               double qualityIncreaseMultiplier, double progressIncreaseMultiplier,
               ActionType type, int activeTurns, int level, bool onGood, bool onExcellent,
               bool onPoor)
    : id(id),
      shortName(shortName),
      fullName(fullName),
      durabilityCost(durabilityCost),
      cpCost(cpCost),
      successProbability(successProbability),
      qualityIncreaseMultiplier(qualityIncreaseMultiplier),
      progressIncreaseMultiplier(progressIncreaseMultiplier),
      type(type),
      activeTurns(activeTurns),
      level(level),
      isConditional(true),
      onGood(onGood),
      onExcellent(onExcellent),
      onPoor(onPoor),
      isCombo(false) {}

Action::Action(ActionId id, const char *shortName, const char *fullName,
               int durabilityCost, int cpCost, double successProbability,
               double qualityIncreaseMultiplier, double progressIncreaseMultiplier,
               ActionType type, int activeTurns, int level,
               std::initializer_list<ActionId> comboActions)
    : id(id),
      shortName(shortName),
      fullName(fullName),
      durabilityCost(durabilityCost),
      cpCost(cpCost),
      successProbability(successProbability),
      qualityIncreaseMultiplier(qualityIncreaseMultiplier),
      progressIncreaseMultiplier(progressIncreaseMultiplier),
      type(type),
      activeTurns(activeTurns),
      level(level),
      isConditional(false),
      onGood(false),
      onExcellent(false),
      onPoor(false),
      isCombo(true),
      comboActions(comboActions) {}