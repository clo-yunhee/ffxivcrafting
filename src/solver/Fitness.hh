#ifndef SOLVER_FITNESS_HH_
#define SOLVER_FITNESS_HH_

struct Fitness {
    double fitness;
    double fitnessProg;
    double cpState;
    int    length;
};

bool        operator<(const Fitness& x, const Fitness& y) noexcept;
inline bool operator>(const Fitness& x, const Fitness& y) noexcept { return y < x; }

#endif  // SOLVER_FITNESS_HH_