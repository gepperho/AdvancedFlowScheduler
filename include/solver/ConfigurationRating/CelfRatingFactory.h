#pragma once

#include "AbstractCelfRating.h"
#include <solver/UtilizationList.h>

namespace celf_rating {

enum class CelfRatingTypes {
    LowID = 0,
    LowPeriodShortPaths = 1,
    EndToEndDelay = 2,
    LowPeriodLowUtilization = 3,
    LowPeriodConfigurationsFirst = 4,
    LowPeriodLongPaths = 5,
};

[[nodiscard]] inline auto to_int(CelfRatingTypes type) -> int
{
    return static_cast<int>(type);
}



[[nodiscard]] auto getRatingStrategy(CelfRatingTypes type,
                                     MultiLayeredGraph& graph,
                                     common::NetworkUtilizationList& network_utilization)
    -> std::unique_ptr<AbstractCelfRating>;


} // namespace celf_rating