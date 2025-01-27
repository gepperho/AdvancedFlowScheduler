#pragma once

#include "AbstractConfigurationRating.h"
#include <solver/UtilizationList.h>

namespace configuration_rating {

enum class ConfigurationRatingTypes {
    BALANCED_NETWORK_UTILIZATION = 0,
    PATH_LENGTH = 1,
    END_TO_END_DELAY = 2,
    BOTTLENECK = 3,
};

[[nodiscard]] inline auto to_int(ConfigurationRatingTypes type) -> int
{
    return static_cast<int>(type);
}

[[nodiscard]] auto getRatingStrategy(ConfigurationRatingTypes type,
                                     MultiLayeredGraph& graph,
                                     common::NetworkUtilizationList& network_utilization)
    -> std::unique_ptr<AbstractConfigurationRating>;


} // namespace configuration_rating