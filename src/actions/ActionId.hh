#ifndef ACTIONS_ACTIONID_HH_
#define ACTIONS_ACTIONID_HH_

#include <cstddef>

enum ActionId {
    Observe = 0,
    BasicSynthesis,
    BasicSynthesis2,
    CarefulSynthesis,
    RapidSynthesis,
    BasicTouch,
    StandardTouch,
    HastyTouch,
    ByregotsBlessing,
    MastersMend,
    TricksOfTheTrade,
    InnerQuiet,
    Manipulation,
    WasteNot,
    WasteNot2,
    Veneration,
    Innovation,
    GreatStrides,
    PreciseTouch,
    MuscleMemory,
    RapidSynthesis2,
    PrudentTouch,
    FocusedSynthesis,
    FocusedTouch,
    Reflect,
    PreparatoryTouch,
    Groundwork,
    DelicateSynthesis,
    IntensiveSynthesis,
    TrainedEye,
    CarefulSynthesis2,
    Groundwork2,
    AdvancedTouch,
    PrudentSynthesis,
    TrainedFinesse,
    // Combo actions:
    FocusedTouchCombo,
    FocusedSynthesisCombo,
    StandardTouchCombo,
    AdvancedTouchCombo,
    // For book-keeping:
    ActionCount,
    NoAction,
};

constexpr size_t ACTION_COUNT = static_cast<size_t>(ActionCount);

#endif  // ACTIONS_ACTIONID_HH_