#ifndef SOLVER_SOLVER_HH_
#define SOLVER_SOLVER_HH_

#include <duthomhas/csprng.hpp>
#include <random>
#include <utility>

#include "Fitness.hh"
#include "Individual.hh"
#include "montecarlo/MonteCarloSim.hh"
#include "simulation/SimSynth.hh"

class SolverSettings;
class Synth;

class Solver {
   public:
    Solver(SolverSettings& settings);

    void solve();

   private:
    Fitness evalSeq(const Individual& individual, const Synth& synth,
                    double penaltyWeight);

    void                              mutateRandomSubSequence(ActionSequence& individual);
    void                              mutateSwap(ActionSequence& individual);
    void                              mutateReverse(ActionSequence& individual);
    void                              mutatePoint(ActionSequence& individual);
    void                              mutateKillSubSequence(ActionSequence& individual);
    Individual                        mutate(const Individual& individual);
    std::pair<Individual, Individual> crossover(const Individual& ind1,
                                                const Individual& ind2);

    std::vector<Individual> selRandom(int k, int startIndex, int endIndex);
    std::vector<Individual> selTournament(int size, int k, int startIndex, int endIndex);

    std::vector<Individual> varCrossover(const std::vector<Individual>& parents,
                                         double                         cxpb);
    void                    varMutate(std::vector<Individual>& offspring, double mutpb);

    void run(const Synth& synth);
    void runOneGen(const Synth& synth);

    bool       isSubPopulationLosing(int subpop);
    bool       hasSubPopulationStagnatedTooMuch(int subpop);
    Individual maxByFitness(const std::vector<Individual>& inds);

    std::array<double, 4> calcPopDiversity();

    double         random();
    int            randomInt(int min, int max);
    ActionId       randomAction();
    ActionSequence randomActionSequence();

    SolverSettings& settings;

    MonteCarloSim _monteCarloSim;
    SimSynth      _simSynth;

    int                     _generationNumber;
    std::vector<Individual> _population;

    Individual          _best;
    std::vector<double> _lastFitnesses;
    std::vector<int>    _lastLeaderboard;
    std::vector<int>    _stagnationCounter;

    // RNG
    duthomhas::csprng                      _seed;
    std::mt19937                           _rng;
    std::uniform_real_distribution<double> _distFloat;
    std::uniform_int_distribution<int32_t> _distInt;
    std::discrete_distribution<int>        _distMut;
    std::discrete_distribution<int>        _distLen1;
};

#endif  // SOLVER_SOLVER_HH_
