#pragma once

class SourceNodeSorting final : public AbstractFlowSorter
{
public:
    explicit SourceNodeSorting(MultiLayeredGraph& graph)
        : graph_(graph),
          flow_starts_(graph.getNumberOfNetworkNodes(), 0)
    {
        std::ranges::for_each(graph.getFlows(), [&](auto& tuple) {
            auto& flow = tuple.second;
            ++flow_starts_.at(flow.source.get());
        });
    }

    auto toString() -> std::string_view override
    {
        return "Source-Node-Sorting";
    }

    auto operator()(const common::FlowNodeID lhs, const common::FlowNodeID rhs) const -> bool override
    {
        auto& lhs_flow = graph_.getFlow(lhs);
        auto& rhs_flow = graph_.getFlow(rhs);
        if(lhs_flow.source.get() != rhs_flow.source.get()) {
            return flow_starts_[lhs_flow.source.get()] < flow_starts_[rhs_flow.source.get()];
        }

        if(lhs_flow.destination.get() != rhs_flow.destination.get()) {
            return lhs_flow.destination.get() < rhs_flow.destination.get();
        }

        const auto lhs_traffic = static_cast<float>(lhs_flow.frame_size) / static_cast<float>(lhs_flow.period);
        const auto rhs_traffic = static_cast<float>(rhs_flow.frame_size) / static_cast<float>(rhs_flow.period);
        if(std::fabs(lhs_traffic - rhs_traffic) > std::numeric_limits<float>::epsilon()) {
            return lhs_traffic < rhs_traffic;
        }
        return lhs_flow.id < rhs_flow.id;
    }

private:
    MultiLayeredGraph& graph_;
    std::vector<int> flow_starts_;
};