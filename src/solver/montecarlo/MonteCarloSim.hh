#ifndef SOLVER_MONTECARLO_MONTECARLOSIM_HH_
#define SOLVER_MONTECARLO_MONTECARLOSIM_HH_

#include <duthomhas/csprng.hpp>
#include <random>

#include "../../model/State.hh"
#include "../ConditionalActionHandling.hh"
#include "../Individual.hh"
#include "MonteCarloStats.hh"

class Action;

class MonteCarloSim {
   public:
    MonteCarloSim();

    State step(const State& startState, const Action& action, bool assumeSuccess,
               bool verbose, bool debug);

    std::vector<State> sequence(const ActionSequence& individual, const State& startState,
                                bool                      assumeSuccess,
                                ConditionalActionHandling conditionalActionHandling,
                                bool verbose, bool debug);

    MonteCarloStats execute(const ActionSequence& individual, const Synth& synth,
                            int nRuns, bool assumeSuccess,
                            ConditionalActionHandling conditionalActionHandling,
                            bool verbose, bool debug);

   private:
    // RNG
    duthomhas::csprng                _seed;
    std::mt19937                     _rng;
    std::uniform_real_distribution<> _dist;

    inline double random() { return _dist(_rng); }

    double qualityPercent(double quality, const Synth& synth) const;
    double qualityFromHqPercent(double hqPercent) const;
    double hqPercentFromQuality(double qualityPercent) const;

    double medianStat(std::vector<MonteCarloStats::Stats>& list,
                      double MonteCarloStats::Stats::*prop) const;
};

#endif  // SOLVER_MONTECARLO_MONTECARLOSIM_HH_