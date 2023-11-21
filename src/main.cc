#include "solver/Solver.hh"
#include "solver/SolverSettings.hh"

int main() {
    // clang-format off
    SolverSettings settings{
        .recipe{
            /*
            // Rarefied Sykon Bavarois
            .baseLevel = 90,
            .level = 560,
            .difficulty = 3500,
            .durability = 80,
            .startQuality = 0,
            .safetyMargin = 0,
            .maxQuality = 7200,
            .suggestedCraftsmanship = 2805,
            .suggestedControl = 2635,
            .progressDivider = 130,
            .progressModifier = 90,
            .qualityDivider = 115,
            .qualityModifier = 80,
            .stars = 0,*/
            // Baked Eggplant
            .baseLevel = 90,
            .level = 640,
            .difficulty = 6600,
            .durability = 70,
            .startQuality = 0,
            .safetyMargin = 0,
            .maxQuality = 14040,
            .suggestedCraftsmanship = 3700,
            .suggestedControl = 3280,
            .progressDivider = 130,
            .progressModifier = 80,
            .qualityDivider = 115,
            .qualityModifier = 70,
            .stars = 4,
        },
        .crafter{
            .cls = Culinarian,
            .level = 90,
            .craftsmanship = 4041,
            .control = 4043,
            .craftingPoints = 611,
            .actions{
                MuscleMemory, Reflect, TrainedEye,
                BasicSynthesis2, CarefulSynthesis2, Groundwork2, PrudentSynthesis,
                DelicateSynthesis,
                FocusedSynthesisCombo, FocusedTouchCombo, StandardTouchCombo, AdvancedTouchCombo,
                BasicTouch, StandardTouch, AdvancedTouch, ByregotsBlessing, PrudentTouch, PreparatoryTouch, TrainedFinesse,
                MastersMend, WasteNot, WasteNot2, Manipulation,
                Veneration, GreatStrides, Innovation,
                Observe,
            },
        },
        .maxTrickUses = 0,
        .reliabilityPercent = 100,
        .maxLength = 0,
        .useConditions = false,
        .solver{
            .population = 8000,
            .generations = 2000,
            .subPopulations = 30,
            .maxStagnationCounter = 20,
            .penaltyWeight = 10000,
            .solveForCompletion = false,
            .remainerCPFitnessValue = 10,
            .remainerDuraFitnessValue = 100,
            .probCrossover = 0.5,
            .probMutation = 0.2,
            .maxSubSeqLength = 4,
        },
        .sequence{},
        .debug = false,
    };
    // clang-format on

    Solver solver(settings);

    solver.solve();

    return 0;
}