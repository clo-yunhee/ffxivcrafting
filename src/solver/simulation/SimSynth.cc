#include "SimSynth.hh"

#include <cmath>
#include <cstdio>

#include "../../actions/Action.hh"
#include "../../actions/ActionTable.hh"
#include "../../model/ConditionModel.hh"
#include "../../model/Crafter.hh"
#include "../../model/Recipe.hh"
#include "../../model/Synth.hh"

State SimSynth::execute(const ActionSequence& individual, const State& startState,
                        bool assumeSuccess, bool verbose, bool debug) {
    // Clone startState to keep it immutable.
    State s(startState);

    // Conditions
    double pGood = s.synth->probabilityOfGood();
    double pExcellent = s.synth->probabilityOfExcellent();
    bool   ignoreConditionReq = !s.synth->useConditions;

    // Step 1 is always normal
    double ppGood = 0;
    double ppExcellent = 0;
    double ppPoor = 0;
    double ppNormal = 1 - (ppGood + ppExcellent + ppPoor);

    ConditionModel simCondition{
        .checkGoodOrExcellent = []() { return true; },
        .pGoodOrExcellent =
            [ignoreConditionReq, &ppGood, &ppExcellent]() {
                return ignoreConditionReq ? 1 : (ppGood + ppExcellent);
            }};

    // Check for empty individuals
    if (individual.empty()) {
        return State(*s.synth);
    }

    if (debug) {
        printf("%-2s %30s %-5s %-5s %-8s %-8s %-5s %-8s %-8s %-5s %-5s %-5s\n", "#",
               "Action", "DUR", "CP", "EQUA", "EPRG", "IQ", "CTL", "QINC", "BPRG", "BQUA",
               "WAC");
        printf("%2d %30s %5.0f %5.0f %8.1f %8.1f %5d %8d %8d %5d %5d %5d\n", s._step, "",
               s._durabilityState, s._cpState, s._qualityState, s._progressState, 0,
               s.synth->crafter.control, 0, 0, 0, 0);
    } else if (verbose) {
        printf("%-2s %30s %-5s %-5s %-8s %-8s %-5s\n", "#", "Action", "DUR", "CP", "EQUA",
               "EPRG", "IQ");
        printf("%2d %30s %5.0f %5.0f %8.1f %8.1f %5d\n", s._step, "", s._durabilityState,
               s._cpState, s._qualityState, s._progressState, 0);
    }

    for (int i = 0; i < individual.size(); ++i) {
        std::vector<const Action*> actionsArray;

        // Combo actions.
        const Action& thisAction = ALL_ACTIONS[individual[i]];
        if (thisAction.isCombo) {
            for (const auto& comboActionId : thisAction.comboActions) {
                actionsArray.push_back(&ALL_ACTIONS[comboActionId]);
            }
        } else {
            actionsArray.push_back(&thisAction);
        }

        for (const auto& action : actionsArray) {
            // Always occurs.
            s._step += 1;

            // Condition calculation.
            double condQualityIncreaseMultiplier = 1.0;
            if (!ignoreConditionReq) {
                condQualityIncreaseMultiplier *=
                    (ppNormal +
                     1.5 * ppGood *
                         std::pow(1 - (ppGood + pGood) / 2, s.synth->maxTrickUses) +
                     4 * ppExcellent + 0.5 * ppPoor);
            }

            // Calculate progress, quality, and durability gains and losses
            // under effects of modifiers.
            ModifiedState r = s.applyModifiers(*action, simCondition);

            // Calculate final gains and losses.
            double successProbability = r.successProbability;
            if (assumeSuccess) {
                successProbability = 1.0;
            }
            double progressGain = r.bProgressGain;
            if (progressGain > 0) {
                s._reliability = s._reliability * successProbability;
            }
            double qualityGain = condQualityIncreaseMultiplier * r.bQualityGain;

            // Floor gains at final stage before calculating expected value.
            progressGain = successProbability * std::floor(progressGain);
            qualityGain = successProbability * std::floor(qualityGain);

            // If a wasted action
            if ((s._progressState >= s.synth->recipe.difficulty) ||
                (s._durabilityState <= 0) || (s._cpState < 0)) {
                s._wastedActions += 1;
            }
            // If not a wasted action
            else {
                s.updateState(*action, progressGain, qualityGain, r.durabilityCost,
                              r.cpCost, simCondition, successProbability);

                // Ending condition update
                if (!ignoreConditionReq) {
                    ppPoor = ppExcellent;
                    ppGood = ppGood * ppNormal;
                    ppExcellent = ppExcellent * ppNormal;
                    ppNormal = 1 - (ppGood + ppExcellent + ppPoor);
                }
            }

            double iqCnt = s._effects.countUps[InnerQuiet].value_or(0.0);
            if (debug) {
                printf(
                    "%2d %30s %5.0f %5.0f %8.1f %8.1f %5.1f %8d %8.1f %5.1f %5.1f "
                    "%5.1f\n",
                    s._step, action->fullName, s._durabilityState, s._cpState,
                    s._qualityState, s._progressState, iqCnt, r.control, qualityGain,
                    std::floor(r.bProgressGain), std::floor(r.bQualityGain),
                    s._wastedActions);
            } else if (verbose) {
                printf("%2d %30s %5.0f %5.0f %8.1f %8.1f %5.1f\n", s._step,
                       action->fullName, s._durabilityState, s._cpState, s._qualityState,
                       s._progressState, iqCnt);
            }

            s._action = action->id;
        }
    }

    // Check for feasibility violations
    bool progressOk, cpOk, durabilityOk, trickOk, reliabilityOk;
    s.checkViolations(progressOk, cpOk, durabilityOk, trickOk, reliabilityOk);

    if (debug || verbose) {
        printf(
            "Progress Check: %s, Durability Check: %s, CP Check: %s, Tricks Check: %s, "
            "Reliability Check: %s, Wasted Actions: %.1f\n",
            bool2str(progressOk), bool2str(durabilityOk), bool2str(cpOk),
            bool2str(trickOk), bool2str(reliabilityOk), s._wastedActions);
    }

    // Return final state
    s._action = individual.back();
    return s;
}