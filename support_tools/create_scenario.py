import argparse
import configparser
import json
import random
import secrets
from datetime import datetime
from math import lcm

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--ini', help='Path to the config file used to create the scenario',
                    type=str, default="create_scenario.ini")
parser.add_argument('-n', '--name', help='name of the output file', type=str)

args = parser.parse_args()


def get_next_cluster_size(delta):
    choices = [x for x in cluster_sizes if x <= delta]
    return random.choice(choices)


def generate_random_flow_cluster(id_counter, cluster_size):
    current_cluster = []
    cluster_device: int = random.choice(end_devices)

    for i in range(0, cluster_size):
        other_device: int = random.choice([x for x in end_devices if x != cluster_device])
        source = cluster_device
        destination = other_device

        if random.randint(0, 1) == 1:
            source = other_device
            destination = cluster_device

        random_period = random.choice(periods)
        random_frame_size = random.choice(frame_sizes)
        if equal_traffic > 0:
            if hyper_cycle % random_period != 0:
                print("Error with hyper-cycle {hc:} and period {p:}".format(hc=hyper_cycle, p=random_period))
            if (125 * equal_traffic) % int(hyper_cycle / random_period) != 0:
                print("Error with total traffic {tt:} and number of packages {np:}"
                      .format(tt=125 * equal_traffic, np=random_period))
            random_frame_size = (125 * equal_traffic) / (hyper_cycle / random_period)

        generated_flow = {'flowID': id_counter + i,
                          'package size': random_frame_size,
                          'period': random_period,
                          'source': source,
                          'destination': destination
                          }
        current_cluster.append(generated_flow)
    return current_cluster


def remove_random_flow_cluster(max_size):
    random_cluster: int = get_random_cluster_index()
    counter = 0
    while len(active_clusters[random_cluster]) > max_size:
        if counter >= 1000:
            break
        counter += 1
        random_cluster = get_random_cluster_index()
    current_cluster = active_clusters[random_cluster]

    if counter >= 1000:
        # delete only a part of the cluster
        active_clusters[random_cluster] = current_cluster[max_size:]
        current_cluster = current_cluster[:max_size]
    else:
        # normal case: delete the whole cluster
        del active_clusters[random_cluster]

    return current_cluster


def get_random_cluster_index():
    random_flow = secrets.randbelow(flow_set_size)

    accumulated_sum = 0
    for i in range(0, len(active_clusters)):
        accumulated_sum += len(active_clusters[i])
        if accumulated_sum > random_flow:
            return i


if __name__ == '__main__':

    # load config
    config = configparser.ConfigParser()
    config.read(args.ini)

    number_of_initial_flows: int = int(config.get('generic', 'number_of_initial_flows'))

    number_of_time_steps: int = int(config.get('generic', 'number_of_time_steps'))
    average_flow_add_per_step: int = int(config.get('generic', 'average_flow_add_per_step'))
    average_flow_remove_per_step: int = int(config.get('generic', 'average_flow_remove_per_step'))
    frame_sizes: [int] = json.loads(config.get('generic', 'frame_sizes'))
    periods: [int] = json.loads(config.get('generic', 'periods'))
    hyper_cycle: int = lcm(*periods)
    equal_traffic: int = int(config.get('generic', 'equal_traffic'))
    cluster_sizes: [int] = json.loads(config.get('generic', 'cluster_sizes'))
    network_path: str = config.get('generic', 'network')

    # create config info object so the file is self containing
    config_info = {
        'number_of_time_steps': number_of_time_steps,
        'number_of_initial_flows': number_of_initial_flows,
        'flow_add': average_flow_add_per_step,
        'flow_remove': average_flow_remove_per_step,
        'package_sizes': frame_sizes,
        'hyper_cycle': hyper_cycle,
        'equal_traffic': equal_traffic,
        'cluster_sizes': cluster_sizes,
        'network_path': network_path,
        'periods': periods
    }

    # load network
    graph = {}
    with open(network_path, 'r') as network_graph_file:
        for line in network_graph_file.readlines():
            if line.startswith('#') or line.startswith('%') or len(line) < 3:
                # filter comments and empty lines
                continue

            separator = '\t'
            if not line.__contains__(separator):
                separator = ' '
            split = line.replace('\r', '').replace('\n', '').split(separator)

            v1: int = int(split[0])
            v2: int = int(split[1])

            if v1 not in graph.keys():
                graph[v1] = []
            if v2 not in graph.keys():
                graph[v2] = []
            graph[v1].append(v2)
            graph[v2].append(v1)

    end_devices = []
    for key, value in graph.items():
        if len(set(value)) == 1:
            end_devices.append(key)

    time_steps = []
    active_clusters: [[int]] = []
    flow_set_size = 0

    # create initial flows
    initial_step = {'time': 0, 'removeFlows': []}
    initial_flows = []
    next_flow_id = 0
    while next_flow_id < number_of_initial_flows:
        current_cluster_size = get_next_cluster_size(number_of_initial_flows - next_flow_id)
        cluster = generate_random_flow_cluster(next_flow_id, current_cluster_size)
        for flow in cluster:
            initial_flows.append(flow)
            next_flow_id += 1
        flow_set_size += len(cluster)
        active_clusters.append(cluster)
    initial_step['addFlows'] = initial_flows
    time_steps.append(initial_step)

    # create later time steps with adds and removes
    next_flow_id = number_of_initial_flows

    for step in range(1, number_of_time_steps):
        # remove clusters
        removes: [int] = []
        removes_to_be_performed = average_flow_remove_per_step
        while removes_to_be_performed > 0:
            removed_cluster = remove_random_flow_cluster(removes_to_be_performed)
            removes_to_be_performed -= len(removed_cluster)
            removes += [x['flowID'] for x in removed_cluster]
            flow_set_size -= len(removed_cluster)

        # add clusters
        adds = []
        internal_counter = 0
        while internal_counter < average_flow_add_per_step:
            next_cluster_size = get_next_cluster_size(average_flow_add_per_step - internal_counter)
            cluster = generate_random_flow_cluster(next_flow_id, next_cluster_size)
            for flow in cluster:
                adds.append(flow)
                # update both counter
                internal_counter += 1
                next_flow_id += 1
            flow_set_size += len(cluster)
            active_clusters.append(cluster)

        time_steps.append({'time': step,
                           'removeFlows': removes,
                           'addFlows': adds})

    # export json
    now = datetime.now()
    graph_name: str = network_path.rsplit('.', 1)[0].rsplit('/', 1)[1] + '_' + now.strftime("%Y_%m_%d-%H_%M") if args.name is None else args.name
    with open('scenario_' + graph_name + '.json', 'w') as outfile:
        json.dump({'time_steps': time_steps,
                   'config_info': config_info}, outfile)
