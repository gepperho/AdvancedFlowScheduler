#pragma once
#include "solver/ConfigurationRating/AbstractConfigurationRating.h"
#include "util/UtilFunctions.h"

class LowPeriodLowUtilization final : public AbstractCelfRating
{
public:
    LowPeriodLowUtilization(MultiLayeredGraph& graph, const common::NetworkUtilizationList& network_utilization)
        : graph_(graph),
          link_utilization_(util::calculateLinkUtilization(graph, network_utilization)),
          hyper_cycle_(util::calculate_hyper_cycle(graph))
    {}

    auto rate(common::ConfigurationNodeID config_id) -> std::pair<float, float> override
    {
        const auto& current_config = graph_.getConfiguration(config_id);
        const auto& current_flow = graph_.getFlow(current_config.flow);

        const auto path_rating = std::transform_reduce(
            current_config.path.begin(), current_config.path.end(),
            1.f,
            std::plus<>(),
            [&](auto link_id) {
                return link_utilization_[link_id];
            });

        auto temp = large_constant / static_cast<float>(current_flow.period)
            + (1.f / path_rating);
        return std::pair(temp, 1.f / static_cast<float>(config_id.get()));
    }

    auto pick(const common::ConfigurationNodeID config_id) -> void override
    {
        const auto& config = graph_.getConfiguration(config_id);
        const auto& flow = graph_.getFlow(config.flow);
        const auto traffic = (hyper_cycle_ / flow.period) * util::calculate_transmission_delay(flow.frame_size);
        for(auto queue_id : config.path) {
            link_utilization_[queue_id] += traffic;
        }
    }

    auto toString() -> std::string_view override
    {
        return "LowPeriodLowUtilizationFirst";
    }

private:
    MultiLayeredGraph& graph_;
    std::unordered_map<common::NetworkQueueID, std::size_t> link_utilization_;
    std::size_t hyper_cycle_;

    const float large_constant = 10000;
};