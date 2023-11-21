#ifndef MODEL_RECIPE_HH_
#define MODEL_RECIPE_HH_

struct Recipe {
    const int baseLevel;
    const int level;
    const int difficulty;
    const int durability;
    const int startQuality;
    const int safetyMargin;
    const int maxQuality;
    const int suggestedCraftsmanship;
    const int suggestedControl;
    const int progressDivider;
    const int progressModifier;
    const int qualityDivider;
    const int qualityModifier;
    const int stars;
};

#endif  // MODEL_RECIPE_HH_
