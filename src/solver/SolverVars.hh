#ifndef SOLVER_SOLVERVARS_HH_
#define SOLVER_SOLVERVARS_HH_

struct SolverVars {
    int    population;
    int    generations;
    int    subPopulations;
    int    maxStagnationCounter;
    double penaltyWeight;
    bool   solveForCompletion;
    double remainerCPFitnessValue;
    double remainerDuraFitnessValue;
    double probCrossover;
    double probMutation;
    int    maxSubSeqLength;
};

#endif  // SOLVER_SOLVERVARS_HH_