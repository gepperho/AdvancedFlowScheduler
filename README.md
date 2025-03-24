# advanced-Flow-scheduler

Code base for flow scheduling with queuing.

When using the code, please cite the following paper:

```
Geppert, H.; Dürr, F.; Bhowmik, S. & Rothermel, K.
Just a Second - Scheduling Thousands of Time-Triggered Streams in Large-Scale Networks
arXiv, 2023
```

## Build

### Requirements

- cmake (3.24 or later)
- C++20 compiler (tested with GCC 13.1.0)
- git
- python 3.9 (or later)

### How to Build

clone the repository and open a shell inside.

```
mkdir build 
cd build
cmake ..
make -j6
```

Now there should be a file called _AdvancedFlowSchedulerExec_.

#### Unit Tests

The unit tests are build by default.
If you do not want to build them, add `-DBUILD_TESTS=0` to the cmake command.

To run the tests, run the executable _unit_tests_ in the test directory.
Run this executable only from within the test directory since otherwise the file paths won't work.

## How to create input Data

You can find some python scripts in the _support_tools_ directory.
To create network graphs, use the `create_network.py` script.
Note: the `create_network.py` script is not very well documented yet. Depending on the graph generator (-g) selected, the other arguments (-s, -d, -e) have different meanings. Check the script itself for details.

When the network graph file is created, it can be visualized with the `draw_graph.py` script.

To create an evaluation scenario, you need a graph file and have to create/modify an ini (default _create_scenario.ini_) file.
Quick config: keep `equal_traffic = 0` and the `cluster_sizes_ = [1]`. The remaining options should be self-explanatory.

Then run the `create_scenario.py` script (pass the ini file as argument).

## How to run the Scheduler

```
./AdvancedFlowSchedulerExec -n "network file path" -s "scenario file path" -a h2s -f 4 -c 1
```

### Command line parameters

| command line option      | description                                                                                                                  |
|--------------------------|------------------------------------------------------------------------------------------------------------------------------|
| -n, --network            | TEXT REQUIRED  File path to the network graph as edge list                                                                   |
| -s, --scenario           | TEXT REQUIRED File path to the scenario as json                                                                              |
| -r, --print-raw          | if set, the results will be printed non pretty for machine parsing                                                           |
| -o, --offensive-planning | if set, the offensive planning will be executed when defensive can not schedule all flows. EDF requires this flag to be set. |

The scheduling and routing can be specified with the following options and arguments.

### Scheduling algorithms [-a, --algorithm]

| Argument | Algorithm                                              |
|----------|--------------------------------------------------------|
| H2S      | Hierarchical Heuristic Scheduling (default)            |
| CELF     | Cost Efficient Lazy Forwarding Version                 |
| EDF      | Earliest Deadline First simulation (requires -o flag)  |
| FF       | First Fit assignment                                   |

### Configuration Placement Strategy [-p, --configuration-placement]

The configuration placement is currently only relevant for H2S and CELF.

| Argument | Placement                           |
|----------|-------------------------------------|
| 0        | ASAP, as soon as possible           |
| 1        | Balanced (default)                  |

### Flow rating (H2S only) [-f, --flow-sorting]

| Argument | Rating Function                  |
|----------|----------------------------------|
| 0        | Highest Traffic Flows First      |
| 1        | Lowest Traffic Flows First       |
| 2        | Lowest ID First                  |
| 3        | Source Node Sorting              |
| 4        | Low Period Flows First (default) |

### Configuration rating (H2S and CELF only) [-c, --configuration-rating]

**For H2S:**

| Argument | Rating Function              |
|----------|------------------------------|
| 0        | Balanced Network Utilization |
| 1        | Path Length (default)        |
| 2        | End to End Delay             |
| 3        | Bottleneck avoidance         |

**For CELF:**

| Argument | Rating Function                  |
|----------|----------------------------------|
| 0        | LowId                            |
| 1        | Low Period Short Paths (default) |
| 2        | End to End Delay                 |
| 3        | Low Period Low Utilization       |
| 4        | Low Period Configurations First  |

### Routing [--routing]

| Argument         | Routing Algorithm                    |
|------------------|--------------------------------------|
| DIJKSTRA_OVERLAP | Dijkstra Overlap algorithm (default) |
| K_SHORTEST       | yen's k-shortest path algorithm      |

### Configurations from the paper

| Algorithm | Flow Rating | Configuration Rating | Placement |
|-----------|-------------|----------------------|-----------|
| H2S       | 4           | 1                    | 1         |
| CELF      | 0           | 3                    | 1         |
| EDF       | -           | -                    | -         |
| FF        | -           | -                    | -         |

## Extending the code

The code is structured in a way that it should be easy to extend it with new scheduling algorithms, routing algorithms, or rating functions.
Each building block has an interface that needs to implemented and a factory where the new implementation needs to be registered.
