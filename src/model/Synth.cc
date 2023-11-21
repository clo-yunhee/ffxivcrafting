#include "Synth.hh"

#include <cmath>
#include <deque>

#include "../actions/ActionTable.hh"
#include "Crafter.hh"
#include "LevelTable.hh"
#include "Recipe.hh"

Synth::Synth(const Crafter &crafter, const Recipe &recipe, int maxTrickUses,
             int reliabilityIndex, bool useConditions, int maxLength,
             const SolverVars &solverVars)
    : crafter(crafter),
      recipe(recipe),
      solverVars(solverVars),
      maxTrickUses(maxTrickUses),
      reliabilityIndex(reliabilityIndex),
      useConditions(useConditions),
      maxLength(maxLength) {}

double Synth::calculateBaseProgressIncrease(int effCrafterLevel,
                                            int craftsmanship) const {
    const double baseValue = (craftsmanship * 10.0) / recipe.progressDivider + 2;
    if (effCrafterLevel <= recipe.level) {
        return std::floor((baseValue * recipe.progressModifier) / 100);
    }
    return std::floor(baseValue);
}

double Synth::calculateBaseQualityIncrease(int effCrafterLevel, int control) const {
    const double baseValue = (control * 10.0) / recipe.qualityDivider + 35;
    if (effCrafterLevel <= recipe.level) {
        return std::floor((baseValue * recipe.qualityModifier) / 100);
    }
    return std::floor(baseValue);
}

double Synth::probabilityOfGood() const {
    int recipeLevel = recipe.level;
    int qualityAssurance = crafter.level >= 63;
    if (recipeLevel >= 300) {  // 70*+
        return qualityAssurance ? 0.11 : 0.10;
    } else if (recipeLevel >= 276) {  // 65+
        return qualityAssurance ? 0.17 : 0.15;
    } else if (recipeLevel >= 255) {  // 61+
        return qualityAssurance ? 0.22 : 0.20;
    } else if (recipeLevel >= 150) {  // 60+
        return qualityAssurance ? 0.11 : 0.10;
    } else if (recipeLevel >= 136) {  // 55+
        return qualityAssurance ? 0.17 : 0.15;
    } else {
        return qualityAssurance ? 0.27 : 0.25;
    }
}

double Synth::probabilityOfExcellent() const {
    int recipeLevel = recipe.level;
    if (recipeLevel >= 300) {  // 70*+
        return 0.01;
    } else if (recipeLevel >= 255) {  // 61+
        return 0.02;
    } else if (recipeLevel >= 150) {  // 60+
        return 0.01;
    } else {
        return 0.02;
    }
}

int Synth::getEffectiveCrafterLevel() const {
    int effCrafterLevel = crafter.level;
    if (LEVEL_TABLE[crafter.level] > 0) {
        effCrafterLevel = LEVEL_TABLE[crafter.level];
    }
    return effCrafterLevel;
}

std::vector<ActionId> Synth::buildHeuristicSequence() const {
    std::deque<ActionId> sequence;
    std::deque<ActionId> subSeq1;
    std::deque<ActionId> subSeq2;
    std::deque<ActionId> subSeq3;

    int cp = crafter.craftingPoints;
    int dur = recipe.durability;
    int progress = 0;
    int quality = 0;

    std::vector<ActionId> preferredActions;
    ActionId              chosenAction;

    // Determine base progress
    int    effCrafterLevel = getEffectiveCrafterLevel();
    int    effRecipeLevel = recipe.level;
    int    levelDifference = effCrafterLevel - effRecipeLevel;
    double bProgressGain =
        calculateBaseProgressIncrease(effCrafterLevel, crafter.craftsmanship);
    double bQualityGain = calculateBaseQualityIncrease(effCrafterLevel, crafter.control);

    double progressGain;
    double qualityGain;

    // Lambdas for convenience
    auto getFirstUsableAction = [&]() {
        for (const ActionId action : preferredActions) {
            if (crafter.hasAction(action)) {
                return action;
            }
        }
        return NoAction;
    };

    auto tryAction = [&](ActionId actionId) {
        const Action &action = ALL_ACTIONS[actionId];
        if (crafter.hasAction(actionId) && cp >= action.cpCost &&
            (dur - action.durabilityCost) >= 0) {
            progressGain = std::floor(bProgressGain * action.progressIncreaseMultiplier);
            qualityGain = std::floor(bQualityGain * action.qualityIncreaseMultiplier);
            chosenAction = actionId;
            return true;
        }
        return false;
    };

    auto tryFirstUsableAction = [&]() {
        for (const ActionId action : preferredActions) {
            if (tryAction(action)) {
                return true;
            }
        }
        return false;
    };

    auto useAction = [&](ActionId actionId) {
        const Action &action = ALL_ACTIONS[actionId];
        cp -= action.cpCost;
        dur -= action.durabilityCost;
        progress += progressGain;
        quality += qualityGain;
    };

    auto pushActionBack = [&](std::deque<ActionId> &seq) {
        seq.push_back(chosenAction);
        useAction(chosenAction);
    };

    auto pushActionFront = [&](std::deque<ActionId> &seq) {
        seq.push_front(chosenAction);
        useAction(chosenAction);
    };

    /* Progress to completion
        -- Determine base progress
        -- Determine best action to use from available list
        -- Steady hand if Careful Synthesis is not available
        -- Master's Mend if more steps are needed
     */

    preferredActions = {FocusedSynthesisCombo, CarefulSynthesis2, CarefulSynthesis,
                        BasicSynthesis2, BasicSynthesis};

    // Final step first.
    if (tryFirstUsableAction()) {
        pushActionBack(subSeq3);
    }

    while (progress < recipe.difficulty) {
        // Don't want to increase progress at 5 durability
        // unless we are able to complete the synth.
        if (tryFirstUsableAction() && dur >= 10) {
            pushActionFront(subSeq2);
        } else if (tryAction(Manipulation)) {
            pushActionFront(subSeq2);
            dur += 30;
        } else if (tryAction(MastersMend)) {
            pushActionFront(subSeq2);
            dur += 30;
        } else {
            break;
        }
    }

    subSeq2.insert(subSeq2.end(), subSeq3.begin(), subSeq3.end());
    sequence.insert(sequence.end(), subSeq2.begin(), subSeq2.end());

    if (dur <= 20) {
        if (tryAction(Manipulation)) {
            pushActionFront(sequence);
            dur += 30;
        } else if (tryAction(MastersMend)) {
            pushActionFront(sequence);
            dur += 30;
        }
    }

    subSeq2.clear();
    subSeq3.clear();

    /* Improve Quality
        -- Reflect and Inner Quiet at the start
        -- Byregot's at the end or other Inner Quiet consumer
     */
    if (tryAction(Reflect)) {
        pushActionBack(subSeq1);
    }

    preferredActions = {AdvancedTouchCombo, StandardTouchCombo, FocusedTouchCombo,
                        BasicTouch};

    // ... and put in at least one quality improving action
    if (tryFirstUsableAction()) {
        pushActionBack(subSeq2);
    }

    subSeq1.insert(subSeq1.end(), subSeq2.begin(), subSeq2.end());

    // Now add in Byregot's Blessing at the end of the quality improving stage if we can.
    if (tryAction(ByregotsBlessing)) {
        pushActionFront(sequence);
    }

    // ... and what the hell, throw in a Great Strides just before it
    if (tryAction(GreatStrides)) {
        pushActionFront(sequence);
    }

    subSeq2.clear();

    // Use up any remaining durability and CP with quality / durability improving actions
    while (cp > 0 && dur > 0 && quality < recipe.maxQuality) {
        if (tryFirstUsableAction() && dur > 10) {
            pushActionBack(subSeq2);
        } else if (dur < 20) {
            if (tryAction(Manipulation)) {
                pushActionFront(subSeq2);
                dur += 30;
            } else if (tryAction(MastersMend)) {
                pushActionFront(subSeq2);
                dur += 30;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    sequence.insert(sequence.begin(), subSeq2.begin(), subSeq2.end());
    sequence.insert(sequence.begin(), subSeq1.begin(), subSeq1.end());

    return std::vector<ActionId>(sequence.begin(), sequence.end());
}