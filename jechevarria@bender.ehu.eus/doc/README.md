# Interconnection Networks Research Flow-level (INRFlow) Extensible Framework

### License Information ###

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.  This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.  You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation, Inc., 51
Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

#### Release Version ####

XXX

## Contributors 

In alphabetical order:

* Alejandro Erickson
* Abbas Kiasari
* Javier Navaridas
* Jose A. Pascual Saiz
* Iain A. Stewart

### Notice to users ###

Please contribute any developments you make on this software back to
its original home.

## Overview of INRFlow ##

INRFlow is a mature, frugal, flow-level simulation framework for modelling large-scale networks and computing systems. 
INRFlow is designed to carry out performance-related studies of interconnection networks for both high performance computing systems and datacentres. 
It features a completely modular design in which adding new topologies, routings or traffic models requires minimum effort. 

INRFlow includes two different simulation engines: 
  * The static engine is able to scale to tens of millions of nodes and provides topological insights.
  * The dynamic engine captures temporal and causal relationships to provide more realistic simulations. 
  
INRFlow has been used to conduct a variety of studies including evaluation of novel topologies and routings (both in the context of graph theory and optimization), analysis of storage and bandwidth allocation strategies and understanding of interferences between application and storage traffic.

INRFlow can be extended by defining new families of topologies and their routing
algorithms (e.g., `src/knkstar/knkstar.c`), and then making minimal changes to the
main INRFlow file (`src/inrflow/main.c`) to let it know about the new topology.

See `inrflow.conf` for examples of accepted input formats, as well as the source
code for the topology you are interested in.

Outputs are written to column separated text files, as well as `stdout` for convenience.

### Release Version Information ###

  * V0.4.0 
	* Dynamic engine added.
	* INRFlow refactored
	* Optional stats and histograms, topology-side
	* New topologies HCN BCN (hcnbcn) and Generalized DCell and FiConn (gdcficonn)
	* New features in Jellyfish topology
	* Shift and Bisection traffic patterns
	* Time measuring infrastructure
  * V0.3.0 First release!
  
## Quick Start ##

INRFlow currently only supports Unix-like systems at the moment. For Windows users we suggest employing a Cygwin environment. 
Older versions worked on Windows environments, but they have less features and are likely to have stability issues.
Use them at your own risk.

From the code/main/ directory, you can compile INRFlow using `make all` and then
execute the binary with `./build/bin/inrflow`.  
The file `inrflow.conf` contains simulation parameters which can be overridden by adding parameters to the command line.

Example execution:

	$ ./build/bin/inrflow topo=bcube_3_3 failure_rate=0.0  tpattern=random_50 generate_bfs=false mode=static

This will run a small Bcube network with 27 servers (3 dimensions with 3 nodes per dimension) generating 50 randomly generated flows using the static engine.
	
The default configuration is defined in `get_conf.c`, at `set_default_conf()`.
Output is written to `.dat` files in the directory the tool is executed,
as well as to `stdout`.

### Static Engine Outputs Explained ###

Stats and histograms are written to `.dat` files, stamped with identifying
information and the time of execution (so you are unlikely to overwrite
anything).  

The same output, stats transposed, is also sent to `stdout`.  
An example output for the execution above follows with some comments:

	inrflow.version                     v0.4.1

Information about the version used to run the simulation
	
	network
	topology                            bcube
	n                                   3
	k                                   3
	topo.version                        v0.1
	n.servers                           27
	n.switches                          27
	switch.radix                        3
	n.links                             162
	n.cables                            81
	p.cable.failures                    0.000000
	n.cable.failures                    0
	n.server.pairs.NN                   729
	n.server.pairs.NN-1                 702
	routing.algorithm                   dimensional

Information about the parameters and characteristics of the simulated topology. 
`servers` refers to the number of endpoints.
`switches` refers to the number of switching elements.
Note that in direct topologies (e.g. torus) servers also function as switches so the number of reported switches is 0.

In our notation, we differentiate between `link` and `cable`: 
`link` refers to each unidirectional channel of the topology, 
whereas `cable` refers to a bidirectional connection between two nodes (servers/switches).

	
	traffic
	pattern                             random
	nrndflows                           50
	n.flows                             50
	n.flows.delivered                   50
	p.flows.connected                   100.000000
	placement.strategy                  sequential

Information about the generated traffic and its parameters.
`n.flows` refers to the total number of flows generated by the traffic pattern.
`n.flows.delivered` is the number of flows that were correctly delivered to their destination.
`p.flows.connected` provides the proportion (in percent).

`placement.strategy` defines a bijection between pattern nodes and network servers.

	miscelanea
	r.seed                              13
	started.at.y.m.d.h.m                2019.08.28.14.46
	on machine                          cspc158
	runtime                             0.000000
	
Some miscelaneous information about the simulation, when and where it was run, and how long it took to complete.
`r.seed` is the random seed parameter of the simulation.

	path.info
	min.link.path.length                0
	max.link.path.length                6
	mean.link.path.length               4.040000
	median.link.path.length             4.000000
	var.link.path.length                3.069091
	std.link.path.length                1.751882

Path length results, including average, median, maximum, minimum, variance and standard deviation. 
We define **path length** as the number of links that have been traversed by a flow. 
Finer grain results are provided as a histogram below.
	
	hop.info
	min.server.hop.length               0
	max.server.hop.length               3
	mean.server.hop.length              2.020000
	median.server.hop.length            2.000000
	var.server.hop.length               0.767273
	std.server.hop.length               0.875941
	
Hop count results, including average, median, maximum, minimum, variance and standard deviation. 
**Hop length** is defined as the number of servers (apart from the source) that have been traversed by a flow.
Only really meaningful for server-centric topologies.
For direct topologies it should be the same as path length.
For indirect topologies it should always be 1 (the destination server).
Finer grain results are provided as a histogram below.

	flow.info
	min.flows.per.link                  0
	max.flows.per.link                  8
	mean.flows.per.link                 2.493827
	median.flows.per.link               2.000000
	var.flows.per.link                  2.189403
	std.flows.per.link                  1.479663
	connected.nonzero.flows.over.bottleneck.flow 87.750000
	connected.flows.over.bottleneck.flow 91.125000
	connected.nonzero.flows.over.mean.flow 281.495050
	connected.flows.over.mean.flow      292.321782
	
Results about the number of flows per link as a measure of congestion in the network.
It includes average, median, maximum, minimum, variance and standard deviation.
Finer grain results are provided as a histogram below.
In addition, if provides the network throughput either considering the `bottleneck` (the most heavy loaded link - **Aggregate Restricted Throughput**) or the `mean` (**Aggregate Non-Restricted Throughput**). 
The `nonzero` results consider Nx(N-1) server pairs (instead of NxN) so to not account for self-sent flows.

	#Server hop length histogram (server hop path length, number of occurrences)
	h    0         5
	h    1        22
	h    2        39
	h    3        34
	#Path length histogram (path length, number of occurrences)
	p    0         5
	p    2        22
	p    4        39
	p    6        34
	#Flows histogram (number of flows, number of uni-directional links with this many flows)
	f    0        14
	f    1        26
	f    2        47
	f    3        35
	f    4        28
	f    5         7
	f    6         4
	f    8         1

Histograms of the hop length, path length and number of flows per link to provided finer grain results than the ones above.

## Extending INRFlow ##

### Adding a new topology ###

INRFlow is designed so that new topologies and routing algorithms can
be added.  We describe how the topology `ficonn` integrates with
INRFlow. Code for `ficonn` resides in `code/main/src/ficonn/`.  In
order for `ficonn` to interface with INRFlow we (1) assigned virtual
functions and (2) defined configuration parameters.

To begin, we copy the **template topology header**, `src/inrflow/topo.h`, to
`src/ficonn/ficonn.h` and implement the functions declared there, renamed as
explained below.  We also remove any optional declarations we will not use (like
extra stat reporting or routing parameters).  Any other functions in ficonn.c
should not be declared in `ficonn.h` and, to be safe, it's best to declare them
as `static` which will avoid any accidental namespace clashes.

(1) We included `ficonn.h` in `main.c`, and assigned its
functions to virtual functions in `init_functions()`.

(2) We added `ficonn` to the list of allowed topologies in
`get_conf.c` at `literal_t topology_l[] = {...}`.  We added the
constant `FICONN` to `typedef enum topo_t {...}` in `misc.h`.

A topology may be implemented with more than one routing algorithm,
e.g., `dpillar`.  These options are added in `get_conf.c` at
`literal_t routing_l[] = {...}`.

Note that `bcube` and `fattree` are the simplest topologies and serve as
templates for implementing new topologies, but comprehensive documentation of
the required functions is provided in the template header file `topo.h`.

#### Topologies with optional stats and histograms ####

Topologies from INRFlow V0.4.x onward can report their own statistics and
histograms to INRflow after the experiment/simulation has ended, which are then
integrated into the normal output.  Examples are `hcnbcn` and `gdcficonn`.  These
functions (below) are a bit delicate, so we have documented them further here.

	// [OPTIONAL] Functions to report custom status from the topology. See, e.g.,  gdcficonn
	long (*get_topo_nstats)();///< number of stats to be reported.
	struct key_value (*get_topo_key_value)(long i);///< ith key value to be used in reporting.c
	long (*get_topo_nhists)();///< number of histograms to be reported
	char (*get_topo_hist_prefix)(long i);///< ith histogram prefix, must avoid 'h' 'p' 'f'
	const char* (*get_topo_hist_doc)(long i);///< ith histogram documentation, latex friendly and not contain '\n'
	long (*get_topo_hist_max)(long i);///< ith histogram length
	void (*get_topo_hist)(long *topo_hist, long i);///< ith histogram (memcpy to topo_hist)

A histogram `H` records the frequency of occurrence of the value at it's index.
That is, `i` happens `H[i]` times.  INRFlow needs:
  * an upper bound on `i` in order to initialise `H`.
  * a doc-string for `H` that does not contain `\n`.  This
    doc-string will appear with `H` in the output.  No limit on length
  * a unique "prefix" character for each histogram, that should be different from the
    ones used in INRFlow, namely, `'h'`, `'p'`, and `'f'`.

Histograms are updated using the function `update_hist_array()`, provided by
`reporting.c`.  Typically we want to glean stats from the histograms, and these
are also provided by `reporting.c`: `X_hist_array()`, where `X` is
`min|max|mean|median|var|std`.  Of course, stats can be generated any which way,
but they are reported using a function from `reporting.c`.  Namely
`make_key_value_node()`.  Return the output of this function in
`get_topo_key_value_yourtopo()`.

### Adding a new traffic pattern ###

Traffic patterns are declared/defined in `traffic.h`/`traffic.c` with an `init_pattern()`
function and a `next_flow()` function. The `init_pattern` function interprets parameters,
precomputes values like the total number of flows that will be generated, and
makes other preparations such as memory allocation. 

The `next_flow()` function is the main function and is called iteratively until it returns an empty flow `(src==-1, dst==-1)`. 
Normally, each flow is generated based on the previous flow that was generated but 
INRFlow provides the freedom to generate flows in many possible ways, such as
pre-populating an array of flows with an arbitrary combinatorial generation
algorithm. Assuming that some care is taken to avoid a time-consuming algorithm,
the developer should note that the memory requirements for storing all flows
might be the bottleneck. INRFlow does not currently do this in the static
engine, where the number of flows might potentially be very large.

Traffic patterns must be added to `get_conf.c` in order that they, and their
parameters, be understood by INRFlow. Add all possible spellings to `literal_t
tpatterns_l[]`. Finally, the virtual functions `init_pattern()` and `next_flow()`
must assigned to the corresponding functions in `traffic.c`; this is done in
`init_functions()` of `main.c`.

## Dynamic execution mode ##

Since v0.4.0 a dynamic execution mode has been added. The dynamic engine allows
INRFlow to simulate real scenarios in which multiple applications share the
network resources. It also simulates the scheduling process allowing the arrival
of new applications when resources freed by other applications become available.

Dynamic engine: This mode considers the bandwidth of the links and the size of
the flows exchanged by the applications. Using this mode, instead of the static,
INRFlow is able to estimate the time required to send and receive all the flows
generated by multiple applications executed concurrently.

Scheduling: It is the process that selects the order in which applications
will be executed following a scheduling policy. It is represented as a list of
applications described with multiple parameters.

### Dynamic Engine Example ###

    inrflow mode=dynamic injmode=x capacity=c1_c2 scheduling=y workload=file_workloadFile

`injmode` (Injection mode): Number of simultaneous flows that a task can inject
into the network. If we set this value to 0, all the flows available will be
injected. Otherwise, only `x` flows can be injected concurrently by each source.

`capacity`: Speed of the links. `c1` corresponds to server links and `c2` to switch
  links.
  
`scheduling` The policy to be used to establish the order in which applications
will be executed. For now, only the `fcfs` (Fist Come First Serve) policy is
implemented.

`workload` It is the set of applications to be executed. Using the file option
INRFlow will load the list of applications from the file `workloadFile`. For now this is the
only way to load the list of applications. The format of the file is as follows:

    0 all2all_1 64 sequential
    10 manyall2allrnd_1_50 10 random
	timestamp application_parameters size allocation 

`timespamp` is the time when the application arrives.

`application` is the name of the kernel or application to be executed.

`parameters` are specific to each pattern or application.

`size` is the number of tasks (communicating nodes) that compose the applications.

`allocation` is the strategy to be used to put the tasks onto the network
  nodes. For now, only  `sequential` and `random` allocations are implemented.

### Extending scheduling, allocation and applications in dynamic mode ###

The addition of new scheduling policies and allocation strategies must be done
in the file `get_conf.c` where new policies and strategies need to be defined in either
`literal_t scheduling_l[]`, `literal_t allocation_l[]` or `literal_t mapping_l[]`. 
Typically this also requires to define a new policy name in `misc.h` (e.g. FCFS).
Implementations will normally go to `scheduling.c` and `allocation.c`. For detailed
information follow the implementation of the existing FCFS scheduling
policy and sequential allocation strategy. These strategies are topology
agnostic. If we want to add specific policies or strategies the procedure is the
same but the recommendation is to put the implementation inside the topology
directory as can be seen in jellyfish with the spread allocation strategy.

The inclusion of new applications (patterns) must be performed in the files
`get_conf.c` and `gen_trace.c`. The former is used to read the application (pattern)
from the configuration file, the latter contains the code to select the
application (pattern). The actual code to generate the flows of the application
is located in the kernels directory. More details can be found in the file
`kernels/collectives.c` where several applications (patterns) are already
implemented.

### Output of the dynamic mode ###

The output of the dynamic mode is organized in four files.

`.scheduling`: Metrics related to the scheduling.
Makespan: Time required to process all the applications. 

`.execution`: Metrics related to the execution of all the application in the workload.
Runtime: Time required to execute all the application. 
Average latency: Average latency of the flows sent by all the applications.
Aggregated bandwidth: Bandwidth used by the flows. 
Number of links shared: This metric measures how many links are used by 0 applications (not used), by 1 application, by 2 applications and so on.

`.applications`: Metrics related to the applications.
Number of the application: The number of applications executed.

`.list_applications`: Contains the metrics for each application (not averaged as in .applications). 
Id: Id of the application.
Arrive time: Time when the application arrived.
Start time: Time when the applicaton started.
Finish time: Time when the application finished.
Runtime: Execution time of the application.
Waiting time: Time that the application waited in the queue.
Number of flows: Number of flows exchanged among the tasks of the applications.
Total distance: Total distance traveled by the flows.
Average flows distance: Average distance traveled by the flows.
Total flows latency: Total latency of all the flows.
Average flows latency: Average latency of the flows.

There is also a verbose mode that will show information about the amount of
flows that are in the network. It can be activated using the option verbose=1.
The option `metricsint` determines when this information is shown. It is
recommended to use the value 0 that selects the interval automatically.

	inrflow mode=dynamic verbose=1 metricsint=0 
