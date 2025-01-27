#pragma once
#include "solver/ConfigurationRating/AbstractConfigurationRating.h"

class PathLengthRating final : public AbstractConfigurationRating
{
public:
    explicit PathLengthRating(MultiLayeredGraph& graph)
        : graph_(graph)
    {}

    auto toString() -> std::string_view override
    {
        return "Path-Length";
    }

    auto rate(const common::ConfigurationNodeID config)
        -> float override
    {
        return static_cast<float>(graph_.getConfiguration(config).path.size());
    }

private:
    MultiLayeredGraph& graph_;
};
