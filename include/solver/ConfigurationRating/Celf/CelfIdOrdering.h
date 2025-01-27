#pragma once
#include "solver/ConfigurationRating/AbstractCelfRating.h"

class CelfIdOrdering final : public AbstractCelfRating
{
public:
    explicit CelfIdOrdering(MultiLayeredGraph& graph)
        : graph_(graph) {}

    auto rate(const common::ConfigurationNodeID config_id) -> std::pair<float, float> override
    {
        const auto& current_config = graph_.getConfiguration(config_id);
        const auto& current_flow = graph_.getFlow(current_config.flow);

        return std::pair(1.f / static_cast<float>(current_flow.id.get()), 1.f / static_cast<float>(config_id.get()));
    }
    auto toString() -> std::string_view override
    {
        return "CelfIdOrdering";
    }

private:
    MultiLayeredGraph& graph_;
};