#pragma once
#include "solver/ConfigurationRating/AbstractCelfRating.h"

class LowPeriodLongPaths final : public AbstractCelfRating
{
public:
    explicit LowPeriodLongPaths(MultiLayeredGraph& graph)
        : graph_(graph) {}

    auto rate(common::ConfigurationNodeID config_id) -> std::pair<float, float> override
    {
        const auto& current_config = graph_.getConfiguration(config_id);
        const auto& current_flow = graph_.getFlow(current_config.flow);

        auto temp = large_constant / static_cast<float>(current_flow.period)
            + static_cast<float>(current_config.path.size());
        return std::pair(temp, 1.f / static_cast<float>(config_id.get()));
    }
    auto toString() -> std::string_view override
    {
        return "Greedy";
    }

private:
    MultiLayeredGraph& graph_;
    const float large_constant = 10000000;
};