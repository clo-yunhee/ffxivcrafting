#ifndef MODEL_CONDITIONMODEL_HH_
#define MODEL_CONDITIONMODEL_HH_

#include <functional>

struct ConditionModel {
    std::function<bool()>   checkGoodOrExcellent;
    std::function<double()> pGoodOrExcellent;
};

#endif  // MODEL_CONDITIONMODEL_HH_