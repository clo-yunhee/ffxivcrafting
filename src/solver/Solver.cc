#include "Solver.hh"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <stdexcept>

#include "../actions/ActionTable.hh"
#include "../model/State.hh"
#include "../model/Synth.hh"
#include "ConditionalActionHandling.hh"
#include "Individual.hh"
#include "SolverSettings.hh"
#include "SolverVars.hh"

using dist_range = std::uniform_int_distribution<int32_t>::param_type;

Solver::Solver(SolverSettings& settings)
    : settings(settings),
      _rng(_seed()),
      _distFloat(0.0, 1.0),
      _distInt(0, INT32_MAX),
      _distMut({
          // randomSubSeq
          60,
          // swap
          10,
          // reverse
          10,
          // randomPoint
          60,
          // killSubSeq
          0,
      }),
      _distLen1({
          // [2-8]
          90,
          // [9-16]
          120,
          // [17-30]
          10,
      }) {}

void Solver::solve() {
    if (settings.maxLength > 0) {
        printf("Maximum length limit of %d is in effect!\n", settings.maxLength);
    }

    printf(
        "Crafter:\n  Class: %s\n  Level: %d\n  Craftsmanship: %d\n  Control: %d\n  "
        "CP: "
        "%d\n  Specialist: %s\n\n",
        class2str(settings.crafter.cls), settings.crafter.level,
        settings.crafter.craftsmanship, settings.crafter.control,
        settings.crafter.craftingPoints, bool2str(settings.crafter.isSpecialist));

    printf(
        "Recipe:\n  Level: %d\n  Difficulty: %d\n  Durability: %d\n  Start Quality: "
        "%d\n "
        " Max Quality: %d\n\n",
        settings.recipe.level, settings.recipe.difficulty, settings.recipe.durability,
        settings.recipe.startQuality, settings.recipe.maxQuality);

    printf(
        "Settings:\n  Max Trick Uses: %d\n  Reliability: %d %% \n  Use Conditions: "
        "%s\n  "
        "Population: %d\n  Generations: %d\n  Penalty Weight: %.0f\n\n",
        settings.maxTrickUses, settings.reliabilityPercent,
        bool2str(settings.useConditions), settings.solver.population,
        settings.solver.generations, settings.solver.penaltyWeight);

    std::vector<std::string> crafterActionNames(settings.crafter.actions.size());
    for (int i = 0; i < settings.crafter.actions.size(); ++i) {
        crafterActionNames[i] = ALL_ACTIONS[settings.crafter.actions[i]].fullName;
    }
    std::sort(crafterActionNames.begin(), crafterActionNames.end());

    if (settings.debug) {
        printf("Crafter Actions:\n");
        for (int i = 0; i < crafterActionNames.size(); ++i) {
            printf("  %d: %s\n", i, crafterActionNames[i].c_str());
        }
    }

    Synth synth(settings.crafter, settings.recipe, settings.maxTrickUses,
                settings.reliabilityPercent / 100.0, settings.useConditions,
                settings.maxLength, settings.solver);
    Synth synthNoConditions(settings.crafter, settings.recipe, settings.maxTrickUses,
                            settings.reliabilityPercent / 100.0, false,
                            settings.maxLength, settings.solver);

    std::vector<ActionId> sequence(settings.sequence);

    bool heuristicGuess = false;
    if (settings.sequence.empty()) {
        heuristicGuess = true;
        sequence = synth.buildHeuristicSequence();

        printf(
            "No initial sequence provided; seeding with the following heuristic "
            "sequence:\n\n");

        for (int i = 0; i < sequence.size(); ++i) {
            if (i + 1 < sequence.size()) {
                printf("%s | ", ALL_ACTIONS[sequence[i]].fullName);
            } else {
                printf("%s", ALL_ACTIONS[sequence[i]].fullName);
            }
        }
        printf("\n\n");

        std::vector<State> states = _monteCarloSim.sequence(
            sequence, State(synth), true, SkipUnusable, false, settings.debug);
        const State& heuristicState = states.back();

        bool progressOk, cpOk, durabilityOk, trickOk, reliabilityOk;
        heuristicState.checkViolations(progressOk, cpOk, durabilityOk, trickOk,
                                       reliabilityOk);

        printf("Heuristic sequence feasibility\n");
        printf(
            "Progress: %s, Durability: %s, CP: %s, Tricks: %s, "
            "Reliability: %s\n",
            bool2str(progressOk), bool2str(durabilityOk), bool2str(cpOk),
            bool2str(trickOk), bool2str(reliabilityOk));
    }

    // Initialize state vectors.
    _best.fitness.fitness = std::numeric_limits<double>::lowest();
    _lastFitnesses.resize(settings.solver.subPopulations);
    _lastLeaderboard.resize(settings.solver.subPopulations);
    _stagnationCounter.resize(settings.solver.subPopulations);

    std::iota(_lastLeaderboard.begin(), _lastLeaderboard.end(), 0);
    std::fill(_stagnationCounter.begin(), _stagnationCounter.end(), 0);

    // Initialize population with the initial guess and random sequences.
    _population = {sequence};
    for (int i = 1; i < settings.solver.population; ++i) {
        _population.emplace_back(randomActionSequence());
    }

    // Initialize fitness for the initial population.
    for (auto& ind : _population) {
        ind.fitness = evalSeq(ind.sequence, synth, settings.solver.penaltyWeight);
    }

    run(synth);

    ActionSequence best = _best.sequence;

    printf("\n\n");
    for (int i = 0; i < best.size(); ++i) {
        printf("%s\n", ALL_ACTIONS[best[i]].fullName);
    }
    printf("\n");

    std::vector<State> states = _monteCarloSim.sequence(
        best, State(synth), true, SkipUnusable, false, settings.debug);
    const State& finalState = states.back();

    bool progressOk, cpOk, durabilityOk, trickOk, reliabilityOk;
    finalState.checkViolations(progressOk, cpOk, durabilityOk, trickOk, reliabilityOk);

    _monteCarloSim.execute(best, synth, 600, false, SkipUnusable, false, settings.debug);
}

Fitness Solver::evalSeq(const Individual& individual, const Synth& synth,
                        double penaltyWeight) {
    State startState(synth);
    State result =
        _simSynth.execute(individual.sequence, startState, false, false, false);
    double penalty(0);
    double fitness(0);
    double fitnessProg(0);
    double safetyMarginFactor(1 + synth.recipe.safetyMargin * 0.01);

    // Sum the constraint violations
    penalty += result._wastedActions / 20;

    // Check for feasability violations
    bool progressOk, cpOk, durabilityOk, trickOk, reliabilityOk;
    result.checkViolations(progressOk, cpOk, durabilityOk, trickOk, reliabilityOk);

    if (!durabilityOk) {
        penalty += std::abs(result._durabilityState);
    }

    if (!progressOk) {
        penalty +=
            std::abs(synth.recipe.difficulty -
                     std::min(result._progressState, double(synth.recipe.difficulty)));
    }

    if (!cpOk) {
        penalty += std::abs(result._cpState);
    }

    if (result._trickUses > synth.maxTrickUses) {
        penalty += std::abs(result._trickUses - synth.maxTrickUses);
    }

    if (result._reliability < synth.reliabilityIndex) {
        penalty += std::abs(synth.reliabilityIndex - result._reliability);
    }

    if (synth.maxLength > 0) {
        int maxActionsExceeded = result._step - synth.maxLength;
        if (maxActionsExceeded > 0) {
            penalty += 0.1 * maxActionsExceeded;
        }
    }

    if (synth.solverVars.solveForCompletion) {
        fitness += result._cpState * synth.solverVars.remainerCPFitnessValue;
        fitness += result._durabilityState * synth.solverVars.remainerDuraFitnessValue;
    } else {
        fitness +=
            std::min(synth.recipe.maxQuality * safetyMarginFactor, result._qualityState);
    }

    fitness -= penaltyWeight * penalty;
    if (progressOk &&
        result._qualityState >= synth.recipe.maxQuality * safetyMarginFactor) {
        // This if statement rewards a smaller synth length
        // so long as conditions are met
        fitness *= (1 + 4.0 / result._step);
    }
    // fitness -= result._cpState * 0.5;  // Penalizes wasted CP
    fitnessProg += result._progressState;

    return {fitness, fitnessProg, result._cpState,
            static_cast<int>(individual.sequence.size())};
}

void Solver::mutateRandomSubSequence(ActionSequence& individual) {
    int maxSubSeqLength =
        std::min(static_cast<int>(individual.size()), settings.solver.maxSubSeqLength);
    int seqLength = randomInt(1, maxSubSeqLength + 1);
    int end = individual.size() - seqLength;
    int start = randomInt(0, end + 1);

    for (int i = start; i <= end; ++i) {
        individual[i] = randomAction();
    }
}

void Solver::mutateSwap(ActionSequence& individual) {
    if (individual.size() >= 2) {
        int i = randomInt(0, individual.size() - 1);
        std::swap(individual[i], individual[i + 1]);
    }
}

void Solver::mutateReverse(ActionSequence& individual) {
    // Reverses a small subselection of actions
    if (individual.size() >= 6) {
        int i = randomInt(0, individual.size() / 2);  // Where to start reversing
        int j = randomInt(0, individual.size() - i);  // How many elements to reverse
        std::reverse(individual.begin() + i, individual.begin() + j + 1);
    }
}

void Solver::mutatePoint(ActionSequence& individual) {
    // Mutates (75%) or kills (25%) a single random action in the sequence.
    int point = randomInt(0, individual.size());
    if (random() < 0.75) {
        individual[point] = randomAction();
    } else {
        individual.erase(individual.begin() + point);
    }
}

void Solver::mutateKillSubSequence(ActionSequence& individual) {
    int maxSubSeqLength =
        std::min(static_cast<int>(individual.size()), settings.solver.maxSubSeqLength);
    int seqLength = randomInt(1, maxSubSeqLength + 1);
    int end = individual.size() - seqLength;
    int start = randomInt(0, end + 1);

    individual.erase(individual.begin() + start, individual.begin() + start + seqLength);
}

Individual Solver::mutate(const Individual& individual) {
    Individual indMut(individual);

    int mut = _distMut(_rng);
    switch (mut) {
        case 0:
            mutateRandomSubSequence(indMut.sequence);
            break;
        case 1:
            mutateSwap(indMut.sequence);
            break;
        case 2:
            mutateRandomSubSequence(indMut.sequence);
            break;
        case 3:
            mutatePoint(indMut.sequence);
            break;
        case 4:
            mutateKillSubSequence(indMut.sequence);
            break;
    }

    // If this killed the individual we'll replace it with a new random sequence.
    if (indMut.sequence.empty()) {
        indMut.sequence = randomActionSequence();
    }

    return indMut;
}

std::pair<Individual, Individual> Solver::crossover(const Individual& ind1,
                                                    const Individual& ind2) {
    int maxInd1 =
        std::min(static_cast<int>(ind1.sequence.size()), settings.solver.maxSubSeqLength);
    int maxInd2 =
        std::min(static_cast<int>(ind2.sequence.size()), settings.solver.maxSubSeqLength);
    int seqLength1 = randomInt(0, maxInd1);
    int seqLength2 = randomInt(0, maxInd2);
    int end1 = ind1.sequence.size() - seqLength1;
    int end2 = ind2.sequence.size() - seqLength2;
    int i1 = randomInt(0, end1 + 1);
    int i2 = randomInt(0, end2 + 1);

    ActionSequence off1(ind1.sequence);
    off1.erase(off1.begin() + i1, off1.begin() + i1 + seqLength1);
    off1.insert(off1.begin() + i1, ind2.sequence.begin() + i2,
                ind2.sequence.begin() + i2 + seqLength2);

    ActionSequence off2(ind2.sequence);
    off2.erase(off2.begin() + i2, off2.begin() + i2 + seqLength2);
    off2.insert(off2.begin() + i2, ind1.sequence.begin() + i1,
                ind1.sequence.begin() + i1 + seqLength1);

    return {off1, off2};
}

void Solver::run(const Synth& synth) {
    printf("\n");

    for (_generationNumber = 1; _generationNumber <= settings.solver.generations;
         ++_generationNumber) {
        runOneGen(synth);

        if (settings.debug) {
            Fitness fitness = evalSeq(_best, synth, settings.solver.penaltyWeight);
            std::array<double, 4> popDiversity = calcPopDiversity();

            printf(
                "Generation [%d]: best fitness = [%.1f, %.1f, %.1f, %d], pop "
                "diversity = [",
                _generationNumber, fitness.fitness, fitness.fitnessProg, fitness.cpState,
                fitness.length);

            for (int i = 0; i < popDiversity.size(); ++i) {
                printf("%.1f", popDiversity[i]);
                if (i + 1 < popDiversity.size()) {
                    printf(", ");
                }
            }

            printf("], pop size: %d\n\n", static_cast<int>(_population.size()));
        } else {
            MonteCarloStats stats = _monteCarloSim.execute(
                _best.sequence, synth, 600, false, SkipUnusable, false, false);
            printf(
                "Gen %5d/%5d -=- Progress: %4d/%4d - Quality: %5d/%5d - CP: %3d/%3d - "
                "Dur: "
                "%3d/%2d - Steps: %2d\r",
                _generationNumber, settings.solver.generations,
                std::min((int)stats.avgStats.progress, synth.recipe.difficulty),
                synth.recipe.difficulty,
                std::min((int)stats.avgStats.quality, synth.recipe.maxQuality),
                synth.recipe.maxQuality, (int)stats.avgStats.cp,
                synth.crafter.craftingPoints, (int)stats.avgStats.durability,
                synth.recipe.durability, (int)_best.sequence.size());
            fflush(stdout);
        }
    }

    if (!settings.debug) {
        printf("\n");
    }
}

std::vector<Individual> Solver::selRandom(int k, int startIndex, int endIndex) {
    std::vector<Individual> r(k);
    for (int i = 0; i < k; ++i) {
        r[i] = _population[randomInt(startIndex, endIndex)];
    }
    return r;
}

std::vector<Individual> Solver::selTournament(int size, int k, int startIndex,
                                              int endIndex) {
    std::vector<Individual> r(k);
    for (int i = 0; i < k; ++i) {
        std::vector<Individual> aspirants = selRandom(size, startIndex, endIndex);
        r[i] = maxByFitness(aspirants);
    }
    return r;
}

std::vector<Individual> Solver::varCrossover(const std::vector<Individual>& parents,
                                             double                         cxpb) {
    std::vector<Individual> offspring(parents);
    for (int i = 1; i < offspring.size(); i += 2) {
        if (random() < cxpb) {
            std::tie(offspring[i - 1], offspring[i]) =
                crossover(offspring[i - 1], offspring[i]);
        }
    }
    return offspring;
}

void Solver::varMutate(std::vector<Individual>& offspring, double mutpb) {
    for (auto& ind : offspring) {
        if (random() < mutpb) {
            ind = mutate(ind);
            // Chance to mutate more.
            for (int i = 0; i < 5; ++i) {
                if (random() < 0.5) {
                    ind = mutate(ind);
                }
            }
        }
    }
}

void Solver::runOneGen(const Synth& synth) {
    // Comparator to sort individuals by decreasing fitness.
    const auto fitComp = [](const auto& x, const auto& y) {
        return x.fitness > y.fitness;
    };

    int subPopulations = settings.solver.subPopulations;

    int    winningSubpop = 0;
    double highestFitness = std::numeric_limits<double>::lowest();

    for (int subpop = 0; subpop < subPopulations; ++subpop) {
        int subpopStartIndex = subpop * _population.size() / subPopulations;
        int subpopEndIndex = (subpop + 1) * _population.size() / subPopulations;
        int subpopLength = subpopEndIndex - subpopStartIndex;

        auto subpopBegin = _population.begin() + subpopStartIndex;
        auto subpopEnd = _population.begin() + subpopEndIndex;

        // If this subpopulation has stagnated for too long,
        // reset with a new random guess.
        // The top third of the subpopulations get 3x as much time to improve.
        if (hasSubPopulationStagnatedTooMuch(subpop)) {
            _stagnationCounter[subpop] = 0;
            Individual random(randomActionSequence());
            random.fitness =
                evalSeq(random.sequence, synth, settings.solver.penaltyWeight);
            std::fill(subpopBegin, subpopEnd, random);
            if (settings.debug) {
                printf("Subpopulation %d has been wiped due to stagnation.\n",
                       subpop + 1);
            }
        }

        // Select parents.
        std::vector<Individual> parents =
            selTournament(7, subpopLength / 2, subpopStartIndex, subpopEndIndex);

        // Breed offspring.
        std::vector<Individual> offspring =
            varCrossover(parents, settings.solver.probCrossover);
        varMutate(offspring, settings.solver.probMutation);

        // Evaluate offspring.
        for (auto& ind : offspring) {
            ind.fitness = evalSeq(ind.sequence, synth, settings.solver.penaltyWeight);
        }

        // Select offspring. Only keep the best half.
        int offspringKeepNum = offspring.size() / 2;
        std::partial_sort(offspring.begin(), offspring.begin() + offspringKeepNum,
                          offspring.end(), fitComp);

        // Select survivors.
        int survivorsKeepNum = subpopLength - offspringKeepNum;
        std::partial_sort(subpopBegin, subpopBegin + survivorsKeepNum, subpopEnd,
                          fitComp);

        // Overwrite the rest of the subpop with the new offspring.
        std::copy(offspring.begin(), offspring.begin() + offspringKeepNum,
                  subpopBegin + survivorsKeepNum);

        // Sort by fitness.
        std::sort(subpopBegin, subpopEnd, fitComp);

        // If the last highest fitness of this subpopulation didn't change enough,
        // increase the stagnation counter.
        if (std::abs(_lastFitnesses[subpop] - subpopBegin->fitness.fitness) < 1e-3) {
            _stagnationCounter[subpop] += 1;
        } else {
            _stagnationCounter[subpop] = 0;
        }

        // Save the last highest fitness of this subpopulation.
        _lastFitnesses[subpop] = subpopBegin->fitness.fitness;
        if (_lastFitnesses[subpop] > highestFitness) {
            highestFitness = _lastFitnesses[subpop];
            winningSubpop = subpop;
            // Save the best.
            _best = *subpopBegin;
        }
    }

    // Save the leaderboard.
    std::sort(_lastLeaderboard.begin(), _lastLeaderboard.end(),
              [this](int i, int j) { return _lastFitnesses[i] > _lastFitnesses[j]; });

    // Debug.
    if (settings.debug) {
        printf("  Winning subpopulation: %d, with fitness %.1f\n", winningSubpop + 1,
               highestFitness);

        printf("  Last fitnesses: [");
        for (int i = 0; i < _lastFitnesses.size(); ++i) {
            printf("%.1f", _lastFitnesses[i]);
            if (i + 1 < _lastFitnesses.size()) {
                printf(", ");
            }
        }
        printf("]\n");

        printf("  Stagnation counter: [");
        for (int i = 0; i < _stagnationCounter.size(); ++i) {
            printf("%d", _stagnationCounter[i]);
            if (i + 1 < _stagnationCounter.size()) {
                printf(", ");
            }
        }
        printf("]\n");

        printf("  Leaderboard: [");
        for (int i = 0; i < _lastLeaderboard.size(); ++i) {
            printf("%d", _lastLeaderboard[i]);
            if (i + 1 < _lastLeaderboard.size()) {
                printf(", ");
            }
        }
        printf("]\n");
    }
}

bool Solver::isSubPopulationLosing(int subpop) {
    // A sub-population is losing if it's in the last third of the leaderboard.
    auto it = std::find(_lastLeaderboard.begin(), _lastLeaderboard.end(), subpop);
    if (it != _lastLeaderboard.end()) {
        return (it - _lastLeaderboard.begin()) >
               std::floor(.66667 * _lastLeaderboard.size());
    }
    return false;
}

bool Solver::hasSubPopulationStagnatedTooMuch(int subpop) {
    // The top two thirds of the subpopulations get 3x as much time to improve.
    if (isSubPopulationLosing(subpop)) {
        return _stagnationCounter[subpop] >= settings.solver.maxStagnationCounter;
    }
    return _stagnationCounter[subpop] >= 3 * settings.solver.maxStagnationCounter;
}

Individual Solver::maxByFitness(const std::vector<Individual>& inds) {
    if (inds.empty()) throw std::invalid_argument("Can't find the max of an empty set;");

    int maxIndex = 0;
    for (int i = 1; i < inds.size(); ++i) {
        if (inds[i].fitness > inds[maxIndex].fitness) {
            maxIndex = i;
        }
    }
    return inds[maxIndex];
}

std::array<double, 4> Solver::calcPopDiversity() {
    std::array<double, 4> avg{0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> var{0.0, 0.0, 0.0, 0.0};

    for (const auto& individual : _population) {
        avg[0] += individual.fitness.fitness;
        avg[1] += individual.fitness.fitnessProg;
        avg[2] += individual.fitness.cpState;
        avg[3] += static_cast<double>(-individual.fitness.length);
    }

    // Average.
    for (auto& x : avg) {
        x /= static_cast<double>(_population.size());
    }

    // Variance.
    for (const auto& individual : _population) {
        var[0] += std::pow(individual.fitness.fitness - avg[0], 2.0);
        var[1] += std::pow(individual.fitness.fitnessProg - avg[1], 2.0);
        var[2] += std::pow(individual.fitness.cpState - avg[2], 2.0);
        var[3] += std::pow(-individual.fitness.length - avg[3], 2.0);
    }

    // Standard deviation.
    for (auto& x : var) {
        x = std::sqrt(x / _population.size());
    }
    return var;
}

double Solver::random() { return _distFloat(_rng); }

int Solver::randomInt(int min, int max) {
    if (max <= min) throw std::invalid_argument("max >= min");
    return _distInt(_rng, dist_range(min, max - 1));
}

ActionId Solver::randomAction() {
    int max = settings.crafter.actions.size();
    return settings.crafter.actions[randomInt(0, max)];
}

ActionSequence Solver::randomActionSequence() {
    int length;

    if (settings.maxLength > 0) {
        length = randomInt(2, settings.maxLength);
    } else {
        // distLen1: [2-8, 9-16, 17-30]
        int lenT = _distLen1(_rng);
        switch (lenT) {
            case 0:  // 2-8
                length = randomInt(2, 9);
                break;
            case 1:  // 9-16
                length = randomInt(9, 17);
                break;
            case 2:  // 17-30
                length = randomInt(17, 31);
                break;
        }
    }

    ActionSequence ind(length);
    for (int i = 0; i < length; ++i) {
        ind[i] = randomAction();
    }
    return ind;
}