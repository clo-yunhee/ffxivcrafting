#ifndef MODEL_STATE_HH_
#define MODEL_STATE_HH_

#include <array>
#include <cstddef>

#include "../actions/ActionId.hh"
#include "Condition.hh"
#include "EffectTracker.hh"

class ConditionModel;
class Synth;
class Action;

constexpr const char *bool2str(bool b) {
    constexpr std::array<const char *, 2> str{"false", "true"};
    return str[static_cast<size_t>(b)];
}

struct ModifiedState {
    const int    craftsmanship;
    const int    control;
    const int    effCrafterLevel;
    const int    effRecipeLevel;
    const int    levelDifference;
    const double successProbability;
    const double qualityIncreaseMultiplier;
    const double bProgressGain;
    const double bQualityGain;
    const double durabilityCost;
    const int    cpCost;
};

class State {
   public:
    State(const Synth &synth);
    State(const State &) = default;
    State &operator=(const State &) = default;

    bool isGoodOrExcellent() const;

   private:
    void checkViolations(bool &progressOk, bool &cpOk, bool &durabilityOk, bool &trickOk,
                         bool &reliabilityOk) const;

    bool useConditionalAction(const ConditionModel &condition);

    ModifiedState applyModifiers(const Action &action, const ConditionModel &condition);

    void applySpecialActionEffects(const Action         &actionId,
                                   const ConditionModel &condition);
    void updateEffectCounters(const Action &actionId, const ConditionModel &condition,
                              double successProbability);

    void updateState(const Action &actionId, double progressGain, double qualityGain,
                     int durabilityCost, int cpCost, const ConditionModel &condition,
                     double successProbability);

    const Synth *synth;

    int           _step;
    int           _lastStep;
    ActionId      _action;
    double        _durabilityState;
    double        _cpState;
    int           _bonusMaxCp;
    double        _qualityState;
    double        _progressState;
    double        _wastedActions;
    int           _trickUses;
    int           _reliability;
    EffectTracker _effects;
    Condition     _condition;
    int           _touchComboStep;

    // Internal state variables set after each step.
    double _iqCnt;
    int    _control;
    double _qualityGain;
    double _bProgressGain;
    double _bQualityGain;
    double _success;
    int    _lastDurabilityCost;

    friend class MonteCarloSim;
    friend class SimSynth;
    friend class Solver;
};

#endif  // MODEL_STATE_HH_