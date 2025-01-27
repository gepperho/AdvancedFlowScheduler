import argparse
import math
import random
import secrets

import networkx as nx

parser = argparse.ArgumentParser()
parser.add_argument('-g', '--graphGenerator', help='Graph generator to be used\ne.g. random, circle, even_circle,',
                    type=str, required=True)
parser.add_argument('-s', '--switches', help='Number of switches/bridges in the graph', type=int, required=True)
parser.add_argument('-d', '--end_devices', help='Number of end devices (with degree 1) in the graph or per bridge',
                    type=int, required=False)
parser.add_argument('-e', '--edges',
                    help='Number of edges/links or a ratio of edges to nodes, depending on the graph type', type=int,
                    required=False)

args = parser.parse_args()

end_nodes: int = args.end_devices
switches: int = args.switches
edges_between_switches = switches - 1
if args.edges is not None:
    edges_between_switches = args.edges

path = "./graph_{}_{}_{}_{}.txt".format(args.graphGenerator, switches, edges_between_switches, end_nodes)


def create_random_graph(n, m, e_nodes):
    """
    creates a random graph with n switches and m edges between switches.
    The end nodes are randomly attached to a switch.
    :param n: number of switches
    :param m: number of edges between switches
    :param e_nodes: number of end nodes
    :return:
    """
    p = 2 * m / pow(n, 2)

    g = nx.gnp_random_graph(n, p)
    for i in range(0, 3000):
        if nx.is_connected(g):
            break
        g = nx.gnp_random_graph(n, p)

    if not nx.is_connected(g):
        print('graph is not connected')
    for i in range(0, e_nodes):
        end_node = i + n
        g.add_edge(end_node, secrets.randbelow(n))
    return nx.Graph(g)


def create_even_random_graph(n, m):
    """
    creates a random graph with n switches and m edges between switches.
    Each switch has one end node attached.
    :param n: number of switches
    :param m: number of edges between switches
    :return:
    """
    g = create_random_graph(n, m, 0)

    end_device = n
    for switch in range(0, n):
        g.add_edge(switch, end_device)
        end_device += 1
    return nx.Graph(g)


def create_circle_graph(n, e_nodes):
    """
    creates a circle graph with n switches and e_nodes end nodes.
    The end nodes are randomly attached to a switch.
    :param n: number of switches
    :param e_nodes: number of end nodes
    :return:
    """
    g = nx.circulant_graph(n, [1, 2])

    for i in range(0, e_nodes):
        end_node = i + n
        g.add_edge(end_node, secrets.randbelow(n))
    return g


def create_circle_graph_with_exactly_x_end_nodes_per_switch(n, linkage, x):
    """
    creates a circle graph with exactly x end nodes per switch
    :param n: number of switches
    :param linkage: number of edges in each direction of the circle
    :param x: number of end nodes per switch
    :return:
    """
    g = nx.circulant_graph(n, [1, linkage])

    for i in range(0, n * x):
        end_node = i + n
        g.add_edge(end_node, end_node % n)
    return g


def create_tree_graph(switch_nodes, end_device_nodes):
    """
    creates a tree graph with the given number of switches and end devices.
    The end devices are only connected to leaf nodes of the switch topology, i.e., the leaves of the tree.
    :param switch_nodes:
    :param end_device_nodes:
    :return:
    """
    g = nx.random_tree(switch_nodes)
    switch_leaf_nodes = [x for x in g.nodes() if g.degree(x) == 1]

    if len(switch_leaf_nodes) > end_device_nodes:
        print("number of 'switch leaf nodes' is to small. Please run the generator again")

    for i in range(0, len(switch_leaf_nodes)):
        g.add_edge(switch_nodes + i, switch_leaf_nodes[i])
    for i in range(len(switch_leaf_nodes), end_device_nodes):
        g.add_edge(switch_nodes + i, random.choice(switch_leaf_nodes))
    return g


def create_waxman_graph(switch_nodes, end_device_nodes):
    """
    creates a waxman graph with the given number of switches and end devices.
    The end devices are only connected to leaf nodes of the switch topology.
    :param switch_nodes:
    :param end_device_nodes:
    :return:
    """
    g = nx.waxman_graph(switch_nodes)

    counter = 0
    while not nx.is_connected(g) and counter < 10000:
        g = nx.waxman_graph(switch_nodes)

    switch_leaf_nodes = [x for x in g.nodes() if g.degree(x) == 1]
    if len(switch_leaf_nodes) > end_device_nodes:
        print("number of 'switch leaf nodes' is to small. Please run the generator again")

    for i in range(0, len(switch_leaf_nodes)):
        # add a single end node per switch with only one edge
        g.add_edge(switch_nodes + i, switch_leaf_nodes[i])
    for i in range(len(switch_leaf_nodes), end_device_nodes):
        g.add_edge(switch_nodes + i, secrets.randbelow(switch_nodes))
    return g


def create_even_mesh_graph(mesh_length, end_device_nodes):
    """
    creates a mesh graph with the same number of end devices per switch. The grid is a square.
    :param mesh_length: length of the mesh in one dimension
    :param end_device_nodes: number of end devices per switch
    :return:
    """
    return create_mesh_graph(mesh_length, mesh_length, end_device_nodes)


def create_mesh_graph(mesh_length_a: int, mesh_length_b: int, end_device_nodes: int):
    """
    uses the switches and edges parameters for the length of the a and b side of the mesh rectangle
    :param mesh_length_a: number of switches in dimension A
    :param mesh_length_b: number of switches in dimension B
    :param end_device_nodes: number of end devices per switch
    :return:
    """
    number_of_switches = mesh_length_a * mesh_length_b
    g = nx.Graph()

    counter = 0
    for current_dim in range(0, mesh_length_a):
        for node_in_dim in range(0, mesh_length_b):
            if node_in_dim < mesh_length_b - 1:
                g.add_edge(counter, counter + 1)
            if current_dim < mesh_length_a - 1:
                g.add_edge(counter, counter + mesh_length_b)
            counter += 1

    end_device_id = number_of_switches
    for switch in range(0, number_of_switches):
        for i in range(0, end_device_nodes):
            g.add_edge(switch, end_device_id)
            end_device_id += 1
    return g


if __name__ == '__main__':
    graph_generator = args.graphGenerator
    graph = nx.Graph
    if graph_generator == 'random':
        graph = create_random_graph(switches, edges_between_switches, end_nodes)
    if graph_generator == 'even_random':
        graph = create_even_random_graph(switches, edges_between_switches)
    elif graph_generator == 'circle' or graph_generator == 'ring':
        graph = create_circle_graph(switches, end_nodes)
    elif graph_generator == 'even_circle' or graph_generator == 'even_ring':
        graph = create_circle_graph_with_exactly_x_end_nodes_per_switch(switches,
                                                                        edges_between_switches,
                                                                        max(int(end_nodes / switches), 1))
    elif graph_generator == 'tree':
        graph = create_tree_graph(switches, end_nodes)
    elif graph_generator == 'waxman':
        graph = create_waxman_graph(switches, end_nodes)
    elif graph_generator == 'even_mesh' or graph_generator == 'even_grid':
        graph = create_even_mesh_graph(int(math.sqrt(switches)), end_nodes)
    elif graph_generator == 'mesh' or graph_generator == 'grid':
        graph = create_mesh_graph(switches, edges_between_switches, end_nodes)

    with open(path, 'w') as out_file:
        print("write to ", path)
        for source, destination in graph.edges:
            out_file.write('{}\t{}\n'.format(source, destination))
