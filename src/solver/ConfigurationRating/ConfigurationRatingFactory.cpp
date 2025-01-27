#include "solver/ConfigurationRating/ConfigurationRatingFactory.h"
#include "solver/ConfigurationRating/Configuration/BalancedNetworkUtilizationRating.h"
#include "solver/ConfigurationRating/Configuration/BottleneckHeuristic.h"
#include "solver/ConfigurationRating/Configuration/EndToEndDelayRating.h"
#include "solver/ConfigurationRating/Configuration/PathLengthRating.h"
#include <memory>


auto configuration_rating::getRatingStrategy(const ConfigurationRatingTypes type,
                                             MultiLayeredGraph& graph,
                                             common::NetworkUtilizationList& network_utilization)
    -> std::unique_ptr<AbstractConfigurationRating>
{
    switch(type) {
        using enum ConfigurationRatingTypes;
    case BALANCED_NETWORK_UTILIZATION:
        return std::make_unique<BalancedNetworkUtilizationRating>(graph, network_utilization);
    case PATH_LENGTH:
        return std::make_unique<PathLengthRating>(graph);
    case END_TO_END_DELAY:
        return std::make_unique<EndToEndDelayRating>(graph, network_utilization);
    case BOTTLENECK:
        return std::make_unique<BottleneckHeuristic>(graph, network_utilization);
    }
    return std::make_unique<PathLengthRating>(graph);
}
