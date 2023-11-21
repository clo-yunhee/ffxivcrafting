#include "State.hh"

#include <cmath>
#include <cstdio>

#include "../actions/Action.hh"
#include "../solver/SolverVars.hh"
#include "ConditionModel.hh"
#include "Crafter.hh"
#include "Recipe.hh"
#include "Synth.hh"

State::State(const Synth &synth)
    : synth(&synth),
      _step(0),
      _lastStep(0),
      _action(NoAction),
      _durabilityState(synth.recipe.durability),
      _cpState(synth.crafter.craftingPoints),
      _bonusMaxCp(0),
      _qualityState(synth.recipe.startQuality),
      _progressState(0),
      _wastedActions(0),
      _trickUses(0),
      _reliability(1),
      _condition(Normal),
      _touchComboStep(0),
      _iqCnt(0),
      _control(0),
      _qualityGain(0),
      _bProgressGain(false),
      _bQualityGain(false),
      _success(false),
      _lastDurabilityCost(0) {}

bool State::isGoodOrExcellent() const {
    return _condition == Good || _condition == Excellent;
}

void State::checkViolations(bool &progressOk, bool &cpOk, bool &durabilityOk,
                            bool &trickOk, bool &reliabilityOk) const {
    progressOk = cpOk = durabilityOk = trickOk = reliabilityOk = false;

    if (_progressState >= synth->recipe.difficulty) {
        progressOk = true;
    }

    if (_cpState >= 0) {
        cpOk = true;
    }

    // Ranged edit -- 10 cost actions that bring you to -5 are now valid
    if ((_durabilityState >= -15) && (_progressState >= synth->recipe.difficulty)) {
        // Special allowed cases that bring it to the negatives:
        if ((_lastDurabilityCost == 10 && _durabilityState == -5) ||
            (_lastDurabilityCost == 20 &&
             (_durabilityState == -5 || _durabilityState == -10 ||
              _durabilityState == -15)) ||
            (_durabilityState >= 0)) {
            durabilityOk = true;
        }
    }

    if (_trickUses <= synth->maxTrickUses) {
        trickOk = true;
    }

    if (_reliability >= synth->reliabilityIndex) {
        reliabilityOk = true;
    }
}

bool State::useConditionalAction(const ConditionModel &condition) {
    if (_cpState > 0 && condition.checkGoodOrExcellent()) {
        _trickUses += 1;
        return true;
    } else {
        _wastedActions += 1.0;
        return false;
    }
}

ModifiedState State::applyModifiers(const Action         &action,
                                    const ConditionModel &condition) {
    // Effect modifiers
    int craftsmanship = synth->crafter.craftsmanship;
    int control = synth->crafter.control;
    int cpCost = action.cpCost;

    // Effects modifying level difference
    int    effCrafterLevel = synth->getEffectiveCrafterLevel();
    int    effRecipeLevel = synth->recipe.level;
    int    levelDifference = effCrafterLevel - effRecipeLevel;
    int    originalLevelDifference = levelDifference;
    int    pureLevelDifference = synth->crafter.level - synth->recipe.baseLevel;
    int    recipeLevel = effRecipeLevel;
    int    stars = synth->recipe.stars;
    double durabilityCost = action.durabilityCost;

    // Effects modifying probability
    double successProbability = action.successProbability;
    if (action.id == FocusedSynthesis || action.id == FocusedTouch) {
        if (_action == Observe) {
            successProbability = 1.0;
        }
    }
    successProbability = std::min(successProbability, 1.0);

    // Advanced Touch Combo
    if (action.id == AdvancedTouch) {
        if (_action == StandardTouch && _touchComboStep == 1) {
            _touchComboStep = 0;
            cpCost = 18;
        }
    }

    // Add combo bonus following Basic Touch
    if (action.id == StandardTouch) {
        if (_action == BasicTouch) {
            cpCost = 18;
            _wastedActions -= 0.05;
            _touchComboStep = 1;
        } else if (_action == StandardTouch) {
            _wastedActions += 0.1;
        }
    }

    // Penalize use of WasteNot during solveForCompletion runs
    if ((action.id == WasteNot || action.id == WasteNot2) &&
        synth->solverVars.solveForCompletion) {
        _wastedActions += 50;
    }

    // Effects modifying progress increase multiplier
    double progressIncreaseMultiplier = 1.0;

    if (action.progressIncreaseMultiplier > 0 &&
        _effects.countDowns[MuscleMemory].has_value()) {
        progressIncreaseMultiplier += 1.0;
        _effects.countDowns[MuscleMemory].reset();
    }

    if (_effects.countDowns[Veneration].has_value()) {
        progressIncreaseMultiplier += 0.5;
    }

    if (action.id == MuscleMemory) {
        if (_step != 1) {
            _wastedActions += 1;
            progressIncreaseMultiplier = 0.0;
            cpCost = 0;
        }
    }

    if (_durabilityState < durabilityCost) {
        if (action.id == Groundwork || action.id == Groundwork2) {
            progressIncreaseMultiplier *= 0.5;
        }
    }

    // Effects modifying quality increase multiplier
    double qualityIncreaseMultiplier = 1.0;
    double qualityIncreaseMultiplierIQ = 1.0;
    // This is calculated seperately because it's multiplicative instead of
    // additive! See: how TeamCraft does it

    if (_effects.countDowns[GreatStrides].has_value() && qualityIncreaseMultiplier > 0) {
        qualityIncreaseMultiplier += 1.0;
    }

    if (_effects.countDowns[Innovation].has_value()) {
        qualityIncreaseMultiplier += 0.5;
    }

    if (_effects.countUps[InnerQuiet].has_value()) {
        qualityIncreaseMultiplierIQ += (0.1 * _effects.countUps[InnerQuiet].value());
        // +1 because buffs start incrementing from 0
    }

    // We can only use Byregot actions when we have at least 1 stack of InnerQuiet
    if (action.id == ByregotsBlessing) {
        if (_effects.countUps[InnerQuiet].has_value() &&
            _effects.countUps[InnerQuiet].value() >= 1) {
            qualityIncreaseMultiplier *=
                1 + std::min(0.2 * _effects.countUps[InnerQuiet].value(), 3.0);
        } else {
            qualityIncreaseMultiplier = 0.0;
        }
    }

    // Calculate base and modified progress gain
    double bProgressGain =
        synth->calculateBaseProgressIncrease(effCrafterLevel, craftsmanship);
    bProgressGain = std::floor(bProgressGain * action.progressIncreaseMultiplier *
                               progressIncreaseMultiplier);

    // Calculate base and modified quality gain
    double bQualityGain = synth->calculateBaseQualityIncrease(effCrafterLevel, control);
    bQualityGain = std::floor(bQualityGain * action.qualityIncreaseMultiplier *
                              qualityIncreaseMultiplier * qualityIncreaseMultiplierIQ);

    // Effects modifying durability cost
    if (_effects.countDowns[WasteNot].has_value() ||
        _effects.countDowns[WasteNot2].has_value()) {
        if (action.id == PrudentTouch) {
            bQualityGain = 0;
            _wastedActions += 1;
        } else if (action.id == PrudentSynthesis) {
            bProgressGain = 0;
            _wastedActions += 1;
        } else {
            durabilityCost *= 0.5;
        }
    }

    // Trained Finesse
    if (action.id == TrainedFinesse) {
        // Not at 10 stacks of IQ -> wasted action.
        if (_effects.countUps[InnerQuiet] != 10) {
            _wastedActions += 1;
            bQualityGain = 0;
        }
    }

    // Effects modifying quality gain directly
    if (action.id == TrainedEye) {
        if (_step == 1 && pureLevelDifference >= 10 && synth->recipe.stars == 0) {
            bQualityGain = synth->recipe.maxQuality;
        } else {
            _wastedActions += 1;
            bQualityGain = 0;
            cpCost = 0;
        }
    }

    // We can only use PreciseTouch when state material condition
    // is Good or Excellent. Default is true for probabilistic method.
    if (action.id == PreciseTouch) {
        if (condition.checkGoodOrExcellent()) {
            bQualityGain *= condition.pGoodOrExcellent();
        } else {
            _wastedActions += 1;
            bQualityGain = 0;
            cpCost = 0;
        }
    }

    if (action.id == Reflect) {
        if (_step != 1) {
            _wastedActions += 1;
            control = 0;
            bQualityGain = 0;
            cpCost = 0;
        }
    }

    return {craftsmanship,
            control,
            effCrafterLevel,
            effRecipeLevel,
            levelDifference,
            successProbability,
            qualityIncreaseMultiplier,
            bProgressGain,
            bQualityGain,
            durabilityCost,
            cpCost};
}

void State::applySpecialActionEffects(const Action         &action,
                                      const ConditionModel &condition) {
    // STEP_02
    // Effect management

    // Special Effect
    if (action.id == MastersMend) {
        _durabilityState += 30;
        if (synth->solverVars.solveForCompletion) {
            _wastedActions += 50;
            // Bad code, but it works.
            // We don't want dur increase in solveForCompletion.
        }
    }

    if (_effects.countDowns[Manipulation].has_value() && _durabilityState > 0 &&
        action.id != Manipulation) {
        _durabilityState += 5;
        if (synth->solverVars.solveForCompletion) {
            _wastedActions += 50;
            // Bad code, but it works.
            // We don't want dur increase in solveForCompletion.
        }
    }

    if (action.id == ByregotsBlessing) {
        if (_effects.countUps[InnerQuiet].has_value()) {
            _effects.countUps[InnerQuiet].reset();
        } else {
            _wastedActions += 1;
        }
    }

    if (action.id == Reflect) {
        if (_step == 1) {
            _effects.countUps[InnerQuiet] = 2;
        } else {
            _wastedActions += 1;
        }
    }

    if (action.qualityIncreaseMultiplier > 0 &&
        _effects.countDowns[GreatStrides].has_value()) {
        _effects.countDowns[GreatStrides].reset();
    }

    // Manage effects with conditional requirements.
    if (action.onExcellent || action.onGood) {
        if (useConditionalAction(condition)) {
            if (action.id == TricksOfTheTrade) {
                _cpState += 20 * condition.pGoodOrExcellent();
            }
        }
    }

    if (action.id == Veneration && _effects.countDowns[Veneration].has_value()) {
        _wastedActions += 1;
    }
    if (action.id == Innovation && _effects.countDowns[Innovation].has_value()) {
        _wastedActions += 1;
    }
}

void State::updateEffectCounters(const Action &action, const ConditionModel &condition,
                                 double successProbability) {
    // STEP_03
    // Countdown / Countup Management

    // Decrement countdowns
    for (auto &countDown : _effects.countDowns) {
        if (countDown.has_value()) {
            countDown.value() -= 1;
            if (countDown.value() == 0) {
                countDown.reset();
            }
        }
    }

    if (_effects.countUps[InnerQuiet].has_value()) {
        // Increment InnerQuiet countups that have conditional requirements
        if (action.id == PreparatoryTouch) {
            _effects.countUps[InnerQuiet].value() += 2;
        }
        // Increment InnerQuiet countups that have conditional requirements
        else if (action.id == PreciseTouch && condition.checkGoodOrExcellent()) {
            _effects.countUps[InnerQuiet].value() +=
                2 * successProbability * condition.pGoodOrExcellent();
        }
        // Increment all other InnerQuiet countups
        else if (action.qualityIncreaseMultiplier > 0 && action.id != Reflect &&
                 action.id != TrainedFinesse) {
            _effects.countUps[InnerQuiet].value() += 1 * successProbability;
        }

        // Cap inner quiet stacks at 9 (10)
        if (_effects.countUps[InnerQuiet].value() > 10) {
            _effects.countUps[InnerQuiet] = 10;
        }
    }

    if (action.type == CountDown) {
        if (action.id == MuscleMemory && _step != 1) {
            _wastedActions += 1;
        } else {
            _effects.countDowns[action.id] = action.activeTurns;
        }
    }
}

void State::updateState(const Action &action, double progressGain, double qualityGain,
                        int durabilityCost, int cpCost, const ConditionModel &condition,
                        double successProbability) {
    // State tracking
    _progressState += progressGain;
    _qualityState += qualityGain;
    _durabilityState -= durabilityCost;
    _lastDurabilityCost = durabilityCost;
    _cpState -= cpCost;
    _lastStep++;
    applySpecialActionEffects(action, condition);
    updateEffectCounters(action, condition, successProbability);

    // Sanity checks for state variables
    if (!synth->solverVars.solveForCompletion) {
        if (_durabilityState >= -5 && _progressState >= synth->recipe.difficulty) {
            _durabilityState = 0;
        }
    }
    _durabilityState = std::min(_durabilityState, double(synth->recipe.durability));
    _cpState = std::min(_cpState, double(synth->crafter.craftingPoints + _bonusMaxCp));
}