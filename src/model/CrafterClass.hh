#ifndef MODEL_CRAFTERCLASS_HH_
#define MODEL_CRAFTERCLASS_HH_

#include <array>
#include <cstddef>

enum CrafterClass {
    Carpenter = 0,
    Blacksmith,
    Armorer,
    Goldsmith,
    Leatherworker,
    Weaver,
    Alchemist,
    Culinarian,
};

constexpr const char *class2str(CrafterClass cls) {
    constexpr std::array<const char *, 8> str{
        "CRP", "BSM", "ARM", "GSM", "LTW", "WVR", "ALC", "CUL",
    };
    return str[static_cast<size_t>(cls)];
}

#endif  // MODEL_CRAFTERCLASS_HH_