#pragma once

#include <Typedefs.h>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace graph_structs {

struct Flow;
struct Configuration;
struct EgressQueue;


struct Flow
{
    common::FlowNodeID id;
    std::size_t frame_size; // in byte
    std::size_t period; // in micro seconds
    common::NetworkNodeID source;
    common::NetworkNodeID destination;

    std::vector<common::ConfigurationNodeID> configurations;
};


[[maybe_unused]] inline void to_json(json& j, const Flow& f)
{
    j = json{{"flowID", f.id.get()},
             {"frame size", f.frame_size},
             {"period", f.period},
             {"source", f.source.get()},
             {"destination", f.destination.get()}};
}

[[maybe_unused]] inline void from_json(const json& j, Flow& f)
{
    std::size_t temp;
    j.at("flowID").get_to(temp);
    f.id = common::FlowNodeID{temp};
    j.at("package size").get_to(f.frame_size);
    j.at("period").get_to(f.period);
    j.at("source").get_to(temp);
    f.source = common::NetworkNodeID{temp};
    j.at("destination").get_to(temp);
    f.destination = common::NetworkNodeID{temp};
}

struct Configuration
{
    // note that the ConfigurationID is only unique within the Flow
    common::ConfigurationNodeID id;
    common::FlowNodeID flow;
    std::vector<common::NetworkQueueID> path;
};

struct EgressQueue
{
    common::NetworkQueueID id;
    common::NetworkNodeID destination;
    std::vector<common::ConfigurationNodeID> used_by;
    bool end_device;
    // utilization information
    // queueing decisions
    // propagation delay
};




} // namespace graph_structs