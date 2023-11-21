#ifndef SOLVER_SIMULATION_SIMSYNTH_HH_
#define SOLVER_SIMULATION_SIMSYNTH_HH_

#include "../../model/State.hh"
#include "../Individual.hh"

class SimSynth {
   public:
    State execute(const ActionSequence& individual, const State& startState,
                  bool assumeSuccess, bool verbose, bool debug);
};

#endif  // SOLVER_SIMULATION_SIMSYNTH_HH_