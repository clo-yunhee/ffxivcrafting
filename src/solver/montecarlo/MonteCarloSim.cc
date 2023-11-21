#include "MonteCarloSim.hh"

#include <cmath>
#include <cstdio>
#include <deque>

#include "../../actions/Action.hh"
#include "../../actions/ActionTable.hh"
#include "../../model/ConditionModel.hh"
#include "../../model/Crafter.hh"
#include "../../model/Recipe.hh"
#include "../../model/Synth.hh"

MonteCarloSim::MonteCarloSim() : _rng(_seed()), _dist(0.0, 1.0) {}

State MonteCarloSim::step(const State& startState, const Action& action,
                          bool assumeSuccess, bool verbose, bool debug) {
    // Clone startState to keep it immutable.
    State s(startState);

    // Conditions
    double pGood = s.synth->probabilityOfGood();
    double pExcellent = s.synth->probabilityOfExcellent();
    bool   ignoreConditionReq = !s.synth->useConditions;
    bool   randomizeConditions = !ignoreConditionReq;

    ConditionModel monteCarloCondition{
        .checkGoodOrExcellent =
            [ignoreConditionReq, &s]() {
                return ignoreConditionReq
                         ? true
                         : (s._condition == Good || s._condition == Excellent);
            },
        .pGoodOrExcellent = []() { return 1; }};

    // Initialize counter
    s._step += 1;

    // Condition evaluation
    double condQualityIncreaseMultiplier = 1;
    switch (s._condition) {
        case Excellent:
            condQualityIncreaseMultiplier *= 4.0;
            break;
        case Good:
            condQualityIncreaseMultiplier *= 1.5;
            break;
        case Poor:
            condQualityIncreaseMultiplier *= 0.5;
            break;
        case Normal:
            condQualityIncreaseMultiplier *= 1.0;
            break;
    }

    // Calculate progress, quality, and durability gains and losses
    // under effects of modifiers.
    ModifiedState r = s.applyModifiers(action, monteCarloCondition);

    // Success or failure
    double success = 0;
    double successRand = random();
    if (0 <= successRand && successRand <= r.successProbability) {
        success = 1;
    }
    if (assumeSuccess) {
        success = 1;
    }

    // Calculate final gains and losses.
    double progressGain = success * r.bProgressGain;
    if (progressGain > 0) {
        s._reliability *= r.successProbability;
    }

    double qualityGain = success * condQualityIncreaseMultiplier * r.bQualityGain;

    // Floor gains at final stage before calculating expected value.
    progressGain = std::floor(progressGain);
    qualityGain = std::floor(qualityGain);

    // If a wasted action
    if ((s._progressState >= s.synth->recipe.difficulty) || (s._durabilityState <= 0) ||
        (s._cpState < 0)) {
        s._wastedActions += 1;
    }
    // If not a wasted action
    else {
        s.updateState(action, progressGain, qualityGain, r.durabilityCost, r.cpCost,
                      monteCarloCondition, success);
    }

    // Ending condtion update
    if (s._condition == Excellent) {
        s._condition = Poor;
    } else if (s._condition == Good || s._condition == Poor) {
        s._condition = Normal;
    } else if (s._condition == Normal) {
        if (randomizeConditions) {
            double condRand = random();
            if (0 <= condRand && condRand < pExcellent) {
                s._condition = Excellent;
            } else if (pExcellent <= condRand && condRand < (pExcellent + pGood)) {
                s._condition = Good;
            } else {
                s._condition = Normal;
            }
        } else {
            s._condition = Normal;
        }
    }

    // Check for feasibility violations
    bool progressOk, cpOk, durabilityOk, trickOk, reliabilityOk;
    s.checkViolations(progressOk, cpOk, durabilityOk, trickOk, reliabilityOk);

    double iqCnt = s._effects.countUps[InnerQuiet].value_or(0.0);

    // Add internal state variables for later output of best and worst cases.
    s._action = action.id;
    s._iqCnt = iqCnt;
    s._control = r.control;
    s._qualityGain = qualityGain;
    s._bProgressGain = std::floor(r.bProgressGain);
    s._bQualityGain = std::floor(r.bQualityGain);
    s._success = success;

    if (debug) {
        printf(
            "%2d %30s %5.0f %5.0f %8.0f %8.0f %5.0f %5d %5.0f %5.0f %5.0f %5.0f %-10s "
            "%5.0f\n",
            s._step, action.fullName, s._durabilityState, s._cpState, s._qualityState,
            s._progressState, s._iqCnt, s._control, s._qualityGain, s._bProgressGain,
            s._bQualityGain, s._wastedActions, condition2str(s._condition), s._success);
    } else if (verbose) {
        printf("%2d %30s %5.0f %5.0f %8.0f %8.0f %5.0f %-10s %5.0f\n", s._step,
               action.fullName, s._durabilityState, s._cpState, s._qualityState,
               s._progressState, s._iqCnt, condition2str(s._condition), s._success);
    }

    return s;
}

std::vector<State> MonteCarloSim::sequence(
    const ActionSequence& individualOrig, const State& startState, bool assumeSuccess,
    ConditionalActionHandling conditionalActionHandling, bool verbose, bool debug) {
    State s(startState);

    // Copy individual to allow repositioning.
    ActionSequence individual(individualOrig);

    // Initialize counter
    int maxConditionUses = 0;

    // Check for empty individuals
    if (individual.empty()) {
        return {startState};
    }

    // Strip TricksOfTrade from individual
    std::deque<ActionId> onExcellentOnlyActions;
    std::deque<ActionId> onGoodOnlyActions;
    std::deque<ActionId> onGoodOrExcellentActions;
    std::deque<ActionId> onPoorOnlyActions;
    ActionSequence       tempIndividual;
    if (conditionalActionHandling == Reposition) {
        for (int i = 0; i < individual.size(); ++i) {
            const ActionId aId = individual[i];
            const Action&  a = ALL_ACTIONS[aId];
            if (a.onExcellent && !a.onGood) {
                onExcellentOnlyActions.push_back(aId);
                maxConditionUses++;
            } else if ((a.onGood && !a.onExcellent) && !a.onPoor) {
                onGoodOnlyActions.push_back(aId);
                maxConditionUses++;
            } else if (a.onGood || a.onExcellent) {
                onGoodOrExcellentActions.push_back(aId);
                maxConditionUses++;
            } else if (a.onPoor && !(a.onExcellent || a.onGood)) {
                onPoorOnlyActions.push_back(aId);
                maxConditionUses++;
            } else {
                tempIndividual.push_back(aId);
            }
        }
        individual = std::move(tempIndividual);
    }

    if (debug) {
        printf("%-2s %30s %-5s %-5s %-8s %-8s %-5s %-5s %-5s %-5s %-5s %-5s %-10s %-5s\n",
               "#", "Action", "DUR", "CP", "QUA", "PRG", "IQ", "CTL", "QINC", "BPRG",
               "BQUA", "WAC", "Cond", "S/F");
        printf(
            "%2d %30s %5.0f %5.0f %8.0f %8.0f %5d %5d %5d %5d %5d %5d %-10s "
            "%5d\n",
            s._step, "", s._durabilityState, s._cpState, s._qualityState,
            s._progressState, 0, s.synth->crafter.control, 0, 0, 0, 0, "Normal", 0);
    } else if (verbose) {
        printf("%-2s %30s %-5s %-5s %-8s %-8s %-5s %-10s %-5s\n", "#", "Action", "DUR",
               "CP", "QUA", "PRG", "IQ", "Cond", "S/F");
        printf("%2d %30s %5.0f %5.0f %8.0f %8.0f %5d %-10s %5d\n", s._step, "",
               s._durabilityState, s._cpState, s._qualityState, s._progressState, 0,
               "Normal", 0);
    }

    std::vector<State> states;

    states.push_back(s);

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
            // Determine if action is usable.
            bool usable = (action->onExcellent && s._condition == Excellent) ||
                          (action->onGood && s._condition == Good) ||
                          (action->onPoor && s._condition == Poor) ||
                          (!action->onExcellent && !action->onGood && !action->onPoor);

            if (conditionalActionHandling == Reposition) {
                // Manually re-add condition dependent action when conditions are met
                if (s._trickUses < maxConditionUses) {
                    if (s._condition == Excellent) {
                        if (!onExcellentOnlyActions.empty()) {
                            s = step(s, ALL_ACTIONS[onExcellentOnlyActions.front()],
                                     assumeSuccess, verbose, debug);
                            onExcellentOnlyActions.pop_front();
                            states.push_back(s);
                        } else if (!onGoodOrExcellentActions.empty()) {
                            s = step(s, ALL_ACTIONS[onGoodOrExcellentActions.front()],
                                     assumeSuccess, verbose, debug);
                            onGoodOrExcellentActions.pop_front();
                            states.push_back(s);
                        }
                    }
                    if (s._condition == Good) {
                        if (!onGoodOnlyActions.empty()) {
                            s = step(s, ALL_ACTIONS[onGoodOnlyActions.front()],
                                     assumeSuccess, verbose, debug);
                            onGoodOnlyActions.pop_front();
                            states.push_back(s);
                        } else if (!onGoodOrExcellentActions.empty()) {
                            s = step(s, ALL_ACTIONS[onGoodOrExcellentActions.front()],
                                     assumeSuccess, verbose, debug);
                            onGoodOrExcellentActions.pop_front();
                            states.push_back(s);
                        }
                    }
                    if (s._condition == Poor) {
                        if (!onPoorOnlyActions.empty()) {
                            s = step(s, ALL_ACTIONS[onPoorOnlyActions.front()],
                                     assumeSuccess, verbose, debug);
                            onPoorOnlyActions.pop_front();
                            states.push_back(s);
                        }
                    }
                }

                // Process the original action as another step
                s = step(s, *action, assumeSuccess, verbose, debug);
                states.push_back(s);
            } else if (conditionalActionHandling == SkipUnusable) {
                // If not usable, record a skipped action without
                // progressing other status counters
                if (!usable) {
                    s = State(s);
                    s._action = action->id;
                    s._wastedActions += 1;
                    states.push_back(s);
                }
                // Otherwise, process action as normal
                else {
                    s = step(s, *action, assumeSuccess, verbose, debug);
                    states.push_back(s);
                }
            } else if (conditionalActionHandling == IgnoreUnusable) {
                // If not usable, skip action effect, progress other status counters
                s = step(s, *action, assumeSuccess, verbose, debug);
                states.push_back(s);
            }
        }
    }

    // Check for feasibility violations
    bool progressOk, cpOk, durabilityOk, trickOk, reliabilityOk;
    s.checkViolations(progressOk, cpOk, durabilityOk, trickOk, reliabilityOk);

    if (debug || verbose) {
        printf(
            "Progress Check: %s, Durability Check: %s, CP Check: %s, Tricks Check: "
            "%s, "
            "Reliability Check: %s, Wasted Actions: %.1f\n",
            bool2str(progressOk), bool2str(durabilityOk), bool2str(cpOk),
            bool2str(trickOk), bool2str(reliabilityOk), s._wastedActions);
    }

    return states;
}

MonteCarloStats MonteCarloSim::execute(
    const ActionSequence& individual, const Synth& synth, int nRuns, bool assumeSuccess,
    ConditionalActionHandling conditionalActionHandling, bool verbose, bool debug) {
    State startState(synth);

    std::vector<State> bestSequenceStates;
    std::vector<State> worstSequenceStates;
    std::vector<State> finalStateTracker;

    for (int i = 0; i < nRuns; ++i) {
        const std::vector<State> states =
            sequence(individual, startState, assumeSuccess, conditionalActionHandling,
                     false, false);
        const auto finalState = states.back();  // copy

        if (bestSequenceStates.empty() ||
            finalState._qualityState > bestSequenceStates.back()._qualityState) {
            bestSequenceStates = std::move(states);
        }

        if (worstSequenceStates.empty() ||
            finalState._qualityState < worstSequenceStates.back()._qualityState) {
            worstSequenceStates = std::move(states);
        }

        finalStateTracker.push_back(std::move(finalState));

        if (verbose) {
            printf("%2d %-20s %5.1f %5.1f %8.1f %5.1f %5.1f\n", i, "MonteCarlo",
                   finalState._durabilityState, finalState._cpState,
                   finalState._qualityState, finalState._progressState,
                   finalState._wastedActions);
        }
    }

    std::vector<MonteCarloStats::Stats> list(finalStateTracker.size());
    MonteCarloStats::Stats              avg{0}, mdn{0}, min{0}, max{0};
    int                                 nHQ{0};
    int                                 nSuccesses{0};

    min.durability = min.cp = min.quality = min.progress = min.hqPercent =
        std::numeric_limits<double>::max();

    bool progressOk, cpOk, durabilityOk, trickOk, reliabilityOk;

    for (int i = 0; i < finalStateTracker.size(); ++i) {
        const State& state = finalStateTracker[i];
        state.checkViolations(progressOk, cpOk, durabilityOk, trickOk, reliabilityOk);

        if (progressOk && durabilityOk && cpOk) {
            double dura = state._durabilityState;
            double cp = state._cpState;
            double qual = state._qualityState;
            double prog = state._progressState;
            double qualPct =
                qualityPercent(std::min(qual, double(synth.recipe.maxQuality)), synth);
            double hqPct = hqPercentFromQuality(qualPct);

            nSuccesses += 1;

            avg.durability += dura;
            avg.cp += cp;
            avg.quality += qual;
            avg.progress += prog;
            if (random() <= hqPct / 100) {
                nHQ += 1;
            }

            list[i].durability = dura;
            list[i].cp = cp;
            list[i].quality = qual;
            list[i].progress = prog;
            list[i].hqPercent = hqPct;

            if (dura < min.durability) min.durability = dura;
            if (cp < min.cp) min.cp = cp;
            if (qual < min.quality) min.quality = qual;
            if (prog < min.progress) min.progress = prog;

            if (dura > max.durability) max.durability = dura;
            if (cp > max.cp) max.cp = cp;
            if (qual > max.quality) max.quality = qual;
            if (prog > max.progress) max.progress = prog;
        }
    }

    avg.durability /= nSuccesses;
    avg.cp /= nSuccesses;
    avg.quality /= nSuccesses;
    avg.progress /= nSuccesses;
    avg.hqPercent = (100.0 * nHQ) / nSuccesses;

    double minQualPct =
        qualityPercent(std::min(min.quality, double(synth.recipe.maxQuality)), synth);
    double maxQualPct =
        qualityPercent(std::min(max.quality, double(synth.recipe.maxQuality)), synth);
    min.hqPercent = hqPercentFromQuality(minQualPct);
    max.hqPercent = hqPercentFromQuality(maxQualPct);

    mdn.durability = medianStat(list, &MonteCarloStats::Stats::durability);
    mdn.cp = medianStat(list, &MonteCarloStats::Stats::cp);
    mdn.quality = medianStat(list, &MonteCarloStats::Stats::quality);
    mdn.progress = medianStat(list, &MonteCarloStats::Stats::progress);
    mdn.hqPercent = medianStat(list, &MonteCarloStats::Stats::hqPercent);

    double successRate = (100.00 * nSuccesses) / finalStateTracker.size();

    if (verbose) {
        printf("%-2s %20s %-5s %-5s %-8s %-5s %-5s\n", "", "", "DUR", "CP", "QUA", "PRG",
               "HQ%");
        printf("%2s %-20s %5.0f %5.0f %8.1f %5.1f %5.1f\n", "##",
               "Expected Value: ", avg.durability, avg.cp, avg.quality, avg.progress,
               avg.hqPercent);
        printf("%2s %-20s %5.0f %5.0f %8.1f %5.1f %5.1f\n", "##",
               "Median Value: ", mdn.durability, mdn.cp, mdn.quality, mdn.progress,
               mdn.hqPercent);
        printf("%2s %-20s %5.0f %5.0f %8.1f %5.1f %5.1f\n", "##",
               "Min Value: ", min.durability, min.cp, min.quality, min.progress,
               min.hqPercent);
        printf("%2s %-20s %5.0f %5.0f %8.1f %5.1f %5.1f\n", "##",
               "Max Value: ", max.durability, max.cp, max.quality, max.progress,
               max.hqPercent);

        printf("\n%2s %-20s %5.1f %%\n", "##", "Success Rate: ", successRate);
        printf("\nMonte Carlo Random Example\n==========================\n");
    }

    sequence(individual, startState, assumeSuccess, conditionalActionHandling, false,
             debug);

    if (verbose) {
        printf("\nMonte Carlo Best Example\n==========================\n");

        printf("%-2s %30s %-5s %-5s %-8s %-8s %-5s %-5s %-5s %-5s %-5s %-5s %-10s %-5s\n",
               "#", "Action", "DUR", "CP", "QUA", "PRG", "IQ", "CTL", "QINC", "BPRG",
               "BQUA", "WAC", "Cond", "S/F");

        for (int i = 0; i < bestSequenceStates.size(); i++) {
            const State& s = bestSequenceStates[i];
            const char*  actionName = ALL_ACTIONS[s._action].fullName;
            printf(
                "%2d %30s %5.0f %5.0f %8.0f %8.0f %5.0f %5d %5.0f %5.0f %5.0f %5.0f "
                "%-10s "
                "%5.0f\n",
                s._step, actionName, s._durabilityState, s._cpState, s._qualityState,
                s._progressState, s._iqCnt, s._control, s._qualityGain, s._bProgressGain,
                s._bQualityGain, s._wastedActions, condition2str(s._condition),
                s._success);
        }

        printf("\nMonte Carlo Worst Example\n==========================\n");

        printf("%-2s %30s %-5s %-5s %-8s %-8s %-5s %-5s %-5s %-5s %-5s %-5s %-10s %-5s\n",
               "#", "Action", "DUR", "CP", "QUA", "PRG", "IQ", "CTL", "QINC", "BPRG",
               "BQUA", "WAC", "Cond", "S/F");

        for (int i = 0; i < worstSequenceStates.size(); i++) {
            const State& s = worstSequenceStates[i];
            const char*  actionName = ALL_ACTIONS[s._action].fullName;
            printf(
                "%2d %30s %5.0f %5.0f %8.0f %8.0f %5.0f %5d %5.0f %5.0f %5.0f %5.0f "
                "%-10s "
                "%5.0f\n",
                s._step, actionName, s._durabilityState, s._cpState, s._qualityState,
                s._progressState, s._iqCnt, s._control, s._qualityGain, s._bProgressGain,
                s._bQualityGain, s._wastedActions, condition2str(s._condition),
                s._success);
        }

        printf("\n");
    }

    return {successRate, avg, mdn, min, max};
}

double MonteCarloSim::qualityPercent(double quality, const Synth& synth) const {
    return quality / synth.recipe.maxQuality * 100;
}

double MonteCarloSim::qualityFromHqPercent(double hqPercent) const {
    double x = hqPercent;
    return -5.6604E-6 * std::pow(x, 4) + 0.0015369705 * std::pow(x, 3) -
           0.1426469573 * std::pow(x, 2) + 5.6122722959 * x - 5.5950384565;
}

double MonteCarloSim::hqPercentFromQuality(double qualityPercent) const {
    double hqPercent = 1;
    if (qualityPercent == 0) {
        hqPercent = 1;
    } else if (qualityPercent >= 100) {
        hqPercent = 100;
    } else {
        while (qualityFromHqPercent(hqPercent) < qualityPercent && hqPercent < 100) {
            hqPercent += 1;
        }
    }
    return hqPercent;
}

double MonteCarloSim::medianStat(std::vector<MonteCarloStats::Stats>& list,
                                 double MonteCarloStats::Stats::*prop) const {
    size_t n = list.size() / 2 + (list.size() % 2 != 0);  // = ceil(list.size() / 2)
    std::nth_element(list.begin(), list.begin() + n, list.end(),
                     [prop](const auto& x, const auto& y) { return x.*prop < y.*prop; });
    return list[n].*prop;
}