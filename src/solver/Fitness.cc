#include "Fitness.hh"

#include <tuple>

bool operator<(const Fitness& x, const Fitness& y) noexcept {
    // length check is gt comparison !!
    return std::tuple(x.fitness, x.fitnessProg, x.cpState, -x.length) <
           std::tuple(y.fitness, y.fitnessProg, y.cpState, -y.length);
}