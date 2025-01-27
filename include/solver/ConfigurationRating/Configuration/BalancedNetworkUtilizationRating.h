#pragma once
#include "util/UtilFunctions.h"

class BalancedNetworkUtilizationRating final : public AbstractConfigurationRating
{
public:
    BalancedNetworkUtilizationRating(MultiLayeredGraph& graph, const common::NetworkUtilizationList& network_utilization)
        : graph_(graph),
          link_utilization_(util::calculateLinkUtilization(graph, network_utilization)){};

    auto toString() -> std::string_view override
    {
        return "Balanced-Network-Utilization";
    }

    auto prepare() -> void override
    {
        average_utilization_ = util::calculateAverageLinkUtilization(link_utilization_);
        hyper_cycle_ = util::calculate_hyper_cycle(graph_);
    }


    auto rate(const common::ConfigurationNodeID config)
        -> float override
    {

        const auto& current_config = graph_.getConfiguration(config);
        const auto& current_flow = graph_.getFlow(current_config.flow);

        const auto current_traffic = util::calculate_transmission_delay(current_flow.frame_size) * (hyper_cycle_ / current_flow.period);
        const auto number_of_links = static_cast<float>(link_utilization_.size());
        const auto updated_avg = (average_utilization_ * number_of_links + static_cast<float>(current_traffic)) / number_of_links;

        const auto path_rating = std::transform_reduce(
            current_config.path.begin(), current_config.path.end(),
            0.f,
            std::plus<>(),
            [&](auto link_id) {
                // consider doing  something smarter...
                const auto updated_traffic = static_cast<float>(link_utilization_[link_id] + current_traffic);
                const auto link_rating = updated_traffic < updated_avg ? 0 : updated_traffic - updated_avg;
                return link_rating;
            });

        return path_rating;
    }

private:
    MultiLayeredGraph& graph_;
    std::unordered_map<common::NetworkQueueID, std::size_t> link_utilization_;
    std::size_t hyper_cycle_ = 0;
    float average_utilization_ = 0.f;
};
