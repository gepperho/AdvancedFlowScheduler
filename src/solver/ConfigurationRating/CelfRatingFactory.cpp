#include "solver/ConfigurationRating/CelfRatingFactory.h"
#include "solver/ConfigurationRating/Celf/CelfEndToEndDelayRating.h"
#include "solver/ConfigurationRating/Celf/CelfIdOrdering.h"
#include "solver/ConfigurationRating/Celf/CelfLowPeriodLongPaths.h"
#include "solver/ConfigurationRating/Celf/CelfLowPeriodShortPaths.h"
#include "solver/ConfigurationRating/Celf/LowPeriodConfigurationsFirst.h"
#include "solver/ConfigurationRating/Celf/LowPeriodLowUtilization.h"
#include <fmt/core.h>
#include <memory>

auto celf_rating::getRatingStrategy(const CelfRatingTypes type,
                                    MultiLayeredGraph& graph,
                                    common::NetworkUtilizationList& network_utilization)
    -> std::unique_ptr<AbstractCelfRating>
{
    switch(type) {
    case CelfRatingTypes::LowPeriodConfigurationsFirst:
        return std::make_unique<LowPeriodConfigurationsFirst>(graph);
    case CelfRatingTypes::LowPeriodLowUtilization:
        return std::make_unique<LowPeriodLowUtilization>(graph, network_utilization);
    case CelfRatingTypes::LowID:
        return std::make_unique<CelfIdOrdering>(graph);
    case CelfRatingTypes::EndToEndDelay:
        return std::make_unique<CelfEndToEndDelayRating>(graph, network_utilization);
    case CelfRatingTypes::LowPeriodShortPaths:
        return std::make_unique<LowPeriodShortPaths>(graph);
    case CelfRatingTypes::LowPeriodLongPaths:
        return std::make_unique<LowPeriodLongPaths>(graph);
    default:
        fmt::print("Configuration rating strategy {} unknown. Falling back to default", to_int(type));
        return std::make_unique<CelfIdOrdering>(graph);
    }
}
