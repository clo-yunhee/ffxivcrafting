#include "ActionTable.hh"

// clang-format off

std::array<Action, ACTION_COUNT> ALL_ACTIONS{
    // id                       , shortName              , fullName                 , durabilityCost, cpCost, success, QIM , PIM, actionType, activeTurns, level(, onGood, onExcellent, onPoor),
    Action(Observe              , "observe"              , "Observe"                , 0             , 7     , 1.0    , 0.0 , 0.0, Immediate , 1          , 13)                                 ,

    Action(BasicSynthesis       , "basicSynth"           , "Basic Synthesis"        , 10            , 0     , 1.0    , 0.0 , 1.0, Immediate , 1          , 1)                                  ,
    Action(BasicSynthesis2      , "basicSynth2"          , "Basic Synthesis"        , 10            , 0     , 1.0    , 0.0 , 1.2, Immediate , 1          , 31)                                 ,
    Action(CarefulSynthesis     , "carefulSynthesis"     , "Careful Synthesis"      , 10            , 7     , 1.0    , 0.0 , 1.5, Immediate , 1          , 62)                                 ,
    Action(RapidSynthesis       , "rapidSynthesis"       , "Rapid Synthesis"        , 10            , 0     , 0.5    , 0.0 , 2.5, Immediate , 1          , 9)                                  ,

    Action(BasicTouch           , "basicTouch"           , "Basic Touch"            , 10            , 18    , 1.0    , 1.0 , 0.0, Immediate , 1          , 5)                                  ,
    Action(StandardTouch        , "standardTouch"        , "Standard Touch"         , 10            , 32    , 1.0    , 1.2 , 0.0 ,Immediate , 1          , 18)                                 ,
    Action(HastyTouch           , "hastyTouch"           , "Hasty Touch"            , 10            , 0     , 0.6    , 1.0 , 0.0, Immediate , 1          , 9)                                  ,
    Action(ByregotsBlessing     , "byregotsBlessing"     , "Byregot's Blessing"     , 10            , 24    , 1.0    , 1.0 , 0.0, Immediate , 1          , 50)                                 ,

    Action(MastersMend          , "mastersMend"          , "Master's Mend"          , 0             , 88    , 1.0    , 0.0 , 0.0, Immediate , 1          , 7)                                  ,
    Action(TricksOfTheTrade     , "tricksOfTheTrade"     , "Tricks of the Trade"    , 0             , 0     , 1.0    , 0.0 , 0.0, Immediate , 1          , 13    , true  , true       , false) ,

    Action(InnerQuiet           , "innerQuiet"           , "Inner Quiet"            , 0             , 18    , 1.0    , 0.0 , 0.0, CountUp   , 1          , 11)                                 ,
    Action(Manipulation         , "manipulation"         , "Manipulation"           , 0             , 96    , 1.0    , 0.0 , 0.0, CountDown , 8          , 65)                                 ,
    Action(WasteNot             , "wasteNot"             , "Waste Not"              , 0             , 56    , 1.0    , 0.0 , 0.0, CountDown , 4          , 15)                                 ,
    Action(WasteNot2            , "wasteNot2"            , "Waste Not II"           , 0             , 98    , 1.0    , 0.0 , 0.0, CountDown , 8          , 47)                                 ,
    Action(Veneration           , "veneration"           , "Veneration"             , 0             , 18    , 1.0    , 0.0 , 0.0, CountDown , 4          , 15)                                 ,
    Action(Innovation           , "innovation"           , "Innovation"             , 0             , 18    , 1.0    , 0.0 , 0.0, CountDown , 4          , 26)                                 ,
    Action(GreatStrides         , "greatStrides"         , "Great Strides"          , 0             , 32    , 1.0    , 0.0 , 0.0, CountDown , 3          , 21)                                 ,

    // Heavensward actions
    Action(PreciseTouch         , "preciseTouch"         , "Precise Touch"          , 10            , 18    , 1.0    , 1.5 , 0.0, Immediate , 1          , 53    , true  , true       , false) ,
    Action(MuscleMemory         , "muscleMemory"         , "Muscle Memory"          , 10            , 6     , 1.0    , 0.0 , 3.0, CountDown , 5          , 54)                                 ,

    // Stormblood actions
    Action(RapidSynthesis2      , "rapidSynthesis2"      , "Rapid Synthesis"        , 10            , 0     , 0.5    , 0.0 , 5.0, Immediate , 1          , 63)                                 ,
    Action(PrudentTouch         , "prudentTouch"         , "Prudent Touch"          , 5             , 25    , 1.0    , 1.0 , 0.0, Immediate , 1          , 66)                                 ,
    Action(FocusedSynthesis     , "focusedSynthesis"     , "Focused Synthesis"      , 10            , 5     , 0.5    , 0.0 , 2.0, Immediate , 1          , 67)                                 ,
    Action(FocusedTouch         , "focusedTouch"         , "Focused Touch"          , 10            , 18    , 0.5    , 1.5 , 0.0, Immediate , 1          , 68)                                 ,
    Action(Reflect              , "reflect"              , "Reflect"                , 10            , 6     , 1.0    , 1.0 , 0.0, Immediate , 1          , 69)                                 ,

    // Shadowbringers actions
    Action(PreparatoryTouch     , "preparatoryTouch"     , "Preparatory Touch"      , 20            , 40    , 1.0    , 2.0 , 0.0, Immediate , 1          , 71)                                 ,
    Action(Groundwork           , "groundwork"           , "Groundwork"             , 20            , 18    , 1.0    , 0.0 , 3.0, Immediate , 1          , 72)                                 ,
    Action(DelicateSynthesis    , "delicateSynthesis"    , "Delicate Synthesis"     , 10            , 32    , 1.0    , 1.0 , 1.0, Immediate , 1          , 76)                                 ,
    Action(IntensiveSynthesis   , "intensiveSynthesis"   , "Intensive Synthesis"    , 10            , 6     , 1.0    , 0.0 , 4.0, Immediate , 1          , 78    , true  , true       , false) ,
    Action(TrainedEye           , "trainedEye"           , "Trained Eye"            , 10            , 250   , 1.0    , 0.0 , 0.0, Immediate , 1          , 80)                                 ,

    // Endwalker
    Action(CarefulSynthesis2    , "carefulSynthesis2"    , "Careful Synthesis"      , 10            , 7     , 1.0    , 0.0 , 1.8, Immediate , 1          , 82)                                 ,
    Action(Groundwork2          , "groundwork2"          , "Groundwork"             , 20            , 18    , 1.0    , 0.0 , 3.6, Immediate , 1          , 86)                                 ,
    Action(AdvancedTouch        , "advancedTouch"        , "Advanced Touch"         , 10            , 46    , 1.0    , 1.5 , 0.0, Immediate , 1          , 84)                                 ,
    Action(PrudentSynthesis     , "prudentSynthesis"     , "Prudent Synthesis"      , 5             , 18    , 1.0    , 0.0 , 1.8, Immediate , 1          , 88)                                 ,
    Action(TrainedFinesse       , "trainedFinesse"       , "Trained Finesse"        , 0             , 32    , 1.0    , 1.0 , 0.0, Immediate , 1          , 90)                                 ,
 
    // Combo actions
    Action(FocusedTouchCombo    , "focusedTouchCombo"    , "Focused Touch Combo"    , 10            , 25    , 1.0    , 1.5 , 0.0, Immediate , 1          , 68, {Observe, FocusedTouch}),
    Action(FocusedSynthesisCombo, "focusedSynthesisCombo", "Focused Synthesis Combo", 10            , 12    , 1.0    , 0.0 , 2.0, Immediate , 1          , 67, {Observe, FocusedSynthesis}),
    Action(StandardTouchCombo   , "standardTouchCombo"   , "Standard Touch Combo"   , 20            , 36    , 1.0    , 2.25, 0.0, Immediate , 1          , 18, {BasicTouch, StandardTouch}),
    Action(AdvancedTouchCombo   , "advancedTouchCombo"   , "Advanced Touch Combo"   , 30            , 54    , 1.0    , 3.75, 0.0, Immediate , 1          , 84, {BasicTouch, StandardTouch, AdvancedTouch}),
};

// clang-format on