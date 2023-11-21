#ifndef MODEL_EFFECTTRACKER_HH_
#define MODEL_EFFECTTRACKER_HH_

#include <array>
#include <optional>

#include "../actions/ActionId.hh"

struct EffectTracker {
    EffectTracker() { countUps[InnerQuiet] = 0; }

    EffectTracker(const EffectTracker&) = default;

    std::array<std::optional<double>, ACTION_COUNT> countUps;
    std::array<std::optional<int>, ACTION_COUNT>    countDowns;
};

#endif  // MODEL_EFFECTTRACKER_HH_