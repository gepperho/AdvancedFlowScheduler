# Support Tools

This is a collection of python scripts to create input data for the AdvancedFlowScheduler.

## Create Network Graphs

use the `create_network_graph.py` script. 
It takes three parameters related to the number of switches, network links, and end devices. 
Depending on the network topology to be created these parameters are used differently. Note that the topologies are always formed by the switches, not the end devices.

Example:
```
python3 create_network_graph.py -g random -s 6 -d 12 -e 12
```

| -g: topology | topology description | -s --switches |-d --end_devices | -e --edges |
|----------------|----------------------|---------------|-----------------|-----|
| random         | Erdős–Rényi random graph with `s` switches and about `e` links in between | number of switches | number of end devices, randomly attached to switches | rough number of network links between switches |
| even_random    | Erdős–Rényi random graph with s switches and about e links in between. Every switch is connected to exactly one end device. | number of switches | -- | rough number of network links between switches |
| circle         | Ring graph of `s` switches where every switch is connected with it's direct neighbors. ring(`s`,2). `d` end devices are randomly attached to the switches. | number of switches | number of end devices | -- |
| even_circle    | Ring graph with equal amount of end devices per switch. Switches are connected to their `e` nearest neighbor switches in every direction. ring(`s`, `e`) | number of switches | `d / s` is the number of end devices per switch (at least 1). Note the integer division. | Linkage of the switches |
| tree           | tree network with `s` switches | number of switches | number of end devices, semi-randomly distributed* | -- |
| waxman         | Waxman random graph with `s` switches and `d` end devices | number of switches | number of end devices | -- |
| even_mesh      | mesh graph with `s`x`s` switches. `d` end devices are connected to every switch | first dimension of the grid | number of end devices per switch | -- |
| mesh           | mesh graph with `s`x`e` switches. `d` end devices are connected to every switch | first dimension of the grid | number of end devices per switch | second dimension of the grid |

<sub><sup>
*The end devices are connected to the leaves of the tree structure first and as soon as every leaf switch has one end device, the remaining end devices are randomly connected to all switches. Note that too small values of `d` can lead to incorrect graph interpretations if not all leaf switches have an end device connected.
</sup></sub>


The networks created with this way can be visualized with the `draw_network.py` script.

Example:
```
python3 draw_network.py -n graph.txt
```

## Create Communication Scenarios

Communication scenarios can be created with the `create_scenario.py` script.
It takes two parameters:
 * -n --name: a name to be appended to the output file name. Note that the file name will be constructed with several other inputs.
 * -i --ini: a path to a configuration file with the parameters of the communication scenario

 The configuration file looks as follows:
 ```python
[generic]
number_of_initial_flows = 500

number_of_time_steps = 21

average_flow_add_per_step = 100
average_flow_remove_per_step = 50

# possible frame sizes in bytes.
# Ethernet limits: 64-1500 bytes
# However, due to the time granularity of 1us, use only the following values:
# 125, 250, 375, 500, 625, 750, 875, 1000, 1125, 1250, 1375, 1500
# these range from 1 to 12 us in their transmission delay
frame_sizes = [125, 250, 500, 750, 1000, 1500]

# possible periods of the flows (in 10^-6 s)
periods = [250, 500, 1000, 2000]

equal_traffic = 0

# create clusters of flows. All flows in a cluster either go to or come from one network node. The list holds the possible cluster sizes
cluster_sizes = [1]

# network graph as undirected edge list
network =network_graphs/graph.txt
 ```

