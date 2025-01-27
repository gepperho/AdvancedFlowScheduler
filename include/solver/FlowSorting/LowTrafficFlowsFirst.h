#pragma once

#include "solver/FlowSorting/AbstractFlowSorter.h"

class LowTrafficFlowsFirst final : public AbstractFlowSorter
{
public:
    explicit LowTrafficFlowsFirst(MultiLayeredGraph& graph)
        : graph_(graph)
    {}

    auto toString() -> std::string_view override
    {
        return "Low-Traffic-Flows-First";
    }

    auto operator()(const common::FlowNodeID lhs, const common::FlowNodeID rhs) const -> bool override
    {
        const auto& lhs_flow = graph_.getFlow(lhs);
        const auto lhs_traffic = static_cast<float>(lhs_flow.frame_size) / static_cast<float>(lhs_flow.period);
        const auto& rhs_flow = graph_.getFlow(rhs);
        const auto rhs_traffic = static_cast<float>(rhs_flow.frame_size) / static_cast<float>(rhs_flow.period);
        if(std::fabs(lhs_traffic - rhs_traffic) > std::numeric_limits<float>::epsilon()) {
            return lhs_traffic > rhs_traffic;
        }
        return lhs_flow.id < rhs_flow.id;
    }


private:
    MultiLayeredGraph& graph_;
};