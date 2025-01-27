#pragma once

#include "ProgramOptions.h"
#include "scenario/TimeStep.h"
#include <fmt/core.h>
#include <fstream>
#include <optional>
#include <graph/GraphStructs.h>
#include <graph/MultiLayeredGraph.h>
#include <nlohmann/json.hpp>
#include <util/UtilFunctions.h>

namespace io {


[[nodiscard]] inline auto extractNextIntegerFromLine(std::string_view& line) noexcept -> std::size_t
{
    if(const auto remove = line.find_first_of("0123456789");
       remove != std::string_view::npos) {
        line.remove_prefix(remove);
    }

    std::size_t node{0};
    auto remove_counter = 0;

    // this can be optimized much more
    for(const char digit : line) {
        if(digit == ':' || digit == ',' || std::isspace(digit)) {
            break;
        }

        node *= 10;
        node += digit - '0';
        remove_counter++;
    }
    line.remove_prefix(remove_counter);

    return node;
}

/**
 * takes the network graph file path from the options, reads the edge list and adds the information into a new MultiLayeredGraph
 * @param network_path path to the network graph file
 * @return new MultiLayeredGraph with the network links present
 */
[[nodiscard]] inline auto parseNetworkGraph(const std::string& network_path) -> std::optional<MultiLayeredGraph>
{
    std::ifstream input_file(network_path, std::ios::in);

    if(not input_file) {
        std::cout << "File not found: " << network_path << std::endl;
        return std::nullopt;
    }

    std::vector<std::vector<common::NetworkNodeID>> adjacency_list;

    // read network graph and store it in an adjacency list
    std::string line;
    while(std::getline(input_file, line)) {
        if(line.rfind('#', 0) == 0
           or line.rfind('%', 0) == 0) {
            continue;
        }
        std::string_view temp_sv{line};
        common::NetworkNodeID node1{extractNextIntegerFromLine(temp_sv)};
        common::NetworkNodeID node2{extractNextIntegerFromLine(temp_sv)};

        // fmt::print("{} - {}\n", node1.get(), node2.get());
        if(adjacency_list.size() < std::max(node1, node2).get() + 1) {
            adjacency_list.resize(std::max(node1, node2).get() + 1);
        }
        if(not util::vector_contains(adjacency_list[node1.get()], node2)) {
            adjacency_list[node1.get()].emplace_back(node2);
        }
        if(not util::vector_contains(adjacency_list[node2.get()], node1)) {
            adjacency_list[node2.get()].emplace_back(node1);
        }
    }

    // transform adjacency list into csr
    MultiLayeredGraph graph;
    std::ranges::for_each(adjacency_list, [&](auto& l) {
        std::sort(l.begin(), l.end());
        graph.insertNetworkDevice(l);
    });

    return graph;
}


inline auto parseScenario(const std::string& scenario_path) -> std::vector<TimeStep>
{
    std::ifstream ifs(scenario_path);
    nlohmann::json jf = nlohmann::json::parse(ifs);

    std::vector<TimeStep> scenario;

    for(const auto& time_step : jf["time_steps"]) {
        TimeStep t;
        t.time = time_step["time"].get<std::size_t>();
        for(const auto& remove_entry : time_step["removeFlows"]) {
            t.remove_flows.emplace_back(remove_entry.get<std::size_t>());
        }

        for(const auto& add_entry : time_step["addFlows"]) {
            t.add_flows.emplace_back(add_entry.get<graph_structs::Flow>());
        }
        scenario.emplace_back(std::move(t));
    }

    return scenario;
}

} // namespace io
