#ifndef SOLVER_MONTECARLO_MONTECARLOSTATS_HH_
#define SOLVER_MONTECARLO_MONTECARLOSTATS_HH_

struct MonteCarloStats {
    struct Stats {
        double durability;
        double cp;
        double quality;
        double progress;
        double hqPercent;
    };

    double successPercent;
    Stats  avgStats;
    Stats  mdnStats;
    Stats  minStats;
    Stats  maxStats;
};

#endif  // SOLVER_MONTECARLO_MONTECARLOSTATS_HH_