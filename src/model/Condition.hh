#ifndef MODEL_CONDITION_HH_
#define MODEL_CONDITION_HH_

#include <array>
#include <cstddef>

enum Condition {
    Normal = 0,
    Good,
    Excellent,
    Poor,
};

constexpr const char *condition2str(Condition cond) {
    constexpr std::array<const char *, 4> str{"Normal", "Good", "Excellent", "Poor"};
    return str[static_cast<size_t>(cond)];
}

#endif  // MODEL_CONDITION_HH_