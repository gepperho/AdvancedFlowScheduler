#pragma once

#include <Typedefs.h>
#include <graph/GraphStructs.h>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

namespace io {

struct TimeStep
{
    std::size_t time;
    std::vector<graph_structs::Flow> add_flows;
    std::vector<common::FlowNodeID> remove_flows;
};


} // namespace io