#pragma once

class BottleneckHeuristic final : public AbstractConfigurationRating
{
public:
    BottleneckHeuristic(MultiLayeredGraph& graph, common::NetworkUtilizationList& network_utilization)
        : graph_(graph),
          utilization_(network_utilization)
    {}

    auto toString() -> std::string_view override
    {
        return "BottleneckHeuristic";
    }

    auto rate(common::ConfigurationNodeID config) -> float override
    {
        const auto& current_config = graph_.getConfiguration(config);
        const auto& current_flow = graph_.getFlow(current_config.flow);
        std::size_t deadline = current_flow.period;

        // ignores the first and last link (end system to switch and switch to end system since they are the )
        const auto bottle_neck_capacity = std::ranges::min_element(std::next(current_config.path.begin()), std::prev(current_config.path.end()), {}, [&](auto& link) {
            return calculate_remaining_link_capacity_until(link, deadline);
        });
        return static_cast<float>(bottle_neck_capacity->get()) + (1 / static_cast<float>(config.get()));
    }


private:
    auto calculate_remaining_link_capacity_until(const common::NetworkQueueID link, const std::size_t deadline) const -> std::size_t
    {
        auto& freeSlots = utilization_.getFreeEgressSlots()[link.get()];
        return std::transform_reduce(freeSlots.begin(), freeSlots.end(), std::size_t{0}, std::plus<std::size_t>(), [&](auto& slot) {
            auto& [first, last] = slot;
            // check if flot is before deadline
            if(last < deadline) {
                return last + 1 - first;
            }
            // check if it is completely after the deadline
            if(first >= deadline) {
                return std::size_t{0};
            }
            // compute the amount before the deadline
            return deadline - first;
        });
    };

    MultiLayeredGraph& graph_;
    common::NetworkUtilizationList& utilization_;
};