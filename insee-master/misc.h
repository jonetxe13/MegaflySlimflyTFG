/**
* @file
* @brief	Some miscellaneus tools & definitions.
*
* Tools for memory allocation & error printing.
* Definition of some useful macros.
* Definition of some enumerations.
*/

#ifndef _misc
#define _misc

//#include <errno.h>
#include <stdio.h>
//#include <stdlib.h>

#include "constants.h"

/**
* Choose a random number in [ 0, m ).
*
* @param m maximum.
* @return A random number.
*/
#define ztm(m) (long) (m * ( (1.0*rand() ) / (RAND_MAX+1.0)))

/**
* Return the absolute value
*
* @param m The number.
* @return The absolute value of m.
*/
#define abs(m) ((m<0) ? (-m) : (m))

/**
* The sign of a numeric value.
*
* For zero, the return value is 1
*/
#define sign(x) (x<0 ? -1 : 1)

#define P_NULL (-1) ///< Definition of a NULL value.

/**
* Definition of the maximum chooser
*
* @param a One number.
* @param b Another One.
* @return The maximum of both.
*/
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

/**
* Definition of the minimum chooser
*
* @param a One number.
* @param b Another One.
* @return The minimum of both.
*/
#ifndef min
#define min(a,b)            (((a) > (b)) ? (b) : (a))
#endif

/**
* Definition of the module of a division
*
* @param a One number.
* @param b Another One.
* @return a mod b.
*/
#ifndef mod
#define mod(a,b)            ((((a)%(b)) <  0 ) ? ((a)%(b)) + (b) : ((a)%(b)) )
#endif

/**
* Definition of boolean values.
*/
typedef enum bool_t {
	B_FALSE = 0, B_TRUE = 1
} bool_t;

/**
* Definition of trace cpu events units.
*/
typedef enum cpu_units_t {
	UNIT_MILLISECONDS=0, UNIT_MICROSECONDS=1, UNIT_NANOSECONDS=2, UNIT_CYCLES=3
} cpu_units_t;

/**
* Definition of all accepted topologies.
*/
typedef enum topo_t {
	// Original fsin topologies
	TORUS, MESH,
	// Proposed/evaluated direct topologies
	MIDIMEW, TWISTED, CIRCULANT, CIRC_PK,
	// SpiNNaker topology
	SPINNAKER,
	// Embraces all direct topologies
	DIRECT,
	// Other quasi-direct topologies
	ICUBE, CUBE,
	DRAGONFLY_ABSOLUTE, DRAGONFLY_RELATIVE, DRAGONFLY_CIRCULANT, DRAGONFLY_NAUTILUS, DRAGONFLY_HELIX, DRAGONFLY_RND, DRAGONFLY_OTHER,
	MEGAFLY,
	SLIMFLY,
	QUASI_DIRECT,
	// Indirect Topologies
	FATTREE, SLIMTREE, THINTREE,
	INDIRECT,
	// Graphs: File defined or generated:
	RRG, EXA, GDBG, KAUTZ
} topo_t;

/**
* Classes of VC management.
*/
typedef enum vc_management_t {
	BUBBLE_MANAGEMENT,		// Normal bubble -- requires just 1 VC
	DALLY_MANAGEMENT,		// Dally management -- requires at least 2 VC
	DOUBLE_MANAGEMENT,		// Double bubble: XY, YX. More options for 3D
	TREE_MANAGEMENT,		// Fattree, thintree, slimtree.
	ICUBE_MANAGEMENT,		// indirect cube.
	GRAPH_NODE_MANAGEMENT,
	GRAPH_PORT_MANAGEMENT,
	GRAPH_NODE_PORT_MANAGEMENT,
	GRAPH_DUMMY_MANAGEMENT,
	GRAPH_INC_HOP_MANAGEMENT,
	VOQ_MANAGEMENT,
	SPANNING_TREE_MANAGEMENT,
    DF_DALLY_MANAGEMENT
} vc_management_t;

/**
* Classes of traffic sources.
*/
typedef enum traffic_pattern_t {
	UNIFORM,	// Random selection of destination
	LOCAL,  // 50% of the traffic goes to [0,1], 25% goes to [2,3], 12.5% goes to [4-7], 12.5% goes further.
	SEMI,		// Uniform traffic in the left-half of a cube-like network
	HOTREGION,	// 1/4 of traffic goes to first 1/8 of network; remaining traffic is uniform
	DISTRIBUTE,	// Starting from node n+1, sent packets to n+1, n+2, n+3, ...
	RSDIST,		// Like DISTRIBUTE, but starting with a random destination
	TRANSPOSE,	// Matrix transpose permutation
	TORNADO,	// Tornado permutation
	COMPLEMENT,	// Bit-complement permutation
	BUTTERFLY,	// Butterfly permutation CHECK CHECK CHECK
	SHUFFLE,	// Perfect shuffle permutation
	REVERSAL,	// Bit-reversal permutation
	TRACE,		// Load from trace file
	POPULATION,	// Load a population of a distribution from a trace file
	HISTOGRAM,	// load the histogram of a distribution from a trace file
	HOTSPOT,	// 5% of traffic goes to the eth connection in (0,0), 95% uniform
	SHIFT,		// Apply a given stride to the source node to calculate the destination node.
	BISECT,	// Pair the nodes (randomly) and make each node in a pair to communicate with each other.
	GROUPSHIFT,	// Random traffic among groups of a given size sending to a given group stride.
	ADV		// Apply a given stride to the source node to calculate the destination node.
} traffic_pattern_t;

/**
* Definition of consumption modes.
*/
typedef enum cons_mode_t {
	SINGLE_CONS, MULTIPLE_CONS
} cons_mode_t;

/**
* Definition of DOR classes.
*/
typedef enum routing_t {
	DIMENSION_ORDER_ROUTING,	// DOR means Dimesion Order Routing.
	DIRECTION_ORDER_ROUTING,	// DOR means Direction Order Routing.
	CUBE_ROUTING,				// Just for cubes.
	STATIC_ROUTING,				// Source static routing for other topologies (including ICUBE) generates a rr in the source.
	RANDOM_ROUTING,				// Non-deterministic static routing for other topologies; generates a rr in the source by choosing a path randomly (depends on the topology).
	SRC_ROUTING,			    // Deterministic static routing based on the source id (makes sense for tree-like only).
	DST_ROUTING,				// Deterministic static routing based on the destination id.
	ARITH_ROUTING,				// Arithmetic routing for other topologies (calculated in a per-hop basis).
	ADAPTIVE_ROUTING,			// Adaptive routing for trees and indirect cube (based on ARITHMETIC). Others may follow
	ICUBE_1M_ROUTING,           // Static DOR in icubes
	ICUBE_4M_ROUTING,           // Static DOR in icubes (4meshes)
	MULTISTAGE_ROUTING,			// Just for multistage.
	CAM_ROUTING,           		// CAMs based routing
	VALIANT,					// Valiant based adaptive routing (only for dragonfly yet)
	QUICK_VALIANT_DUAL,				// A variant of Valiant for Dragonflies where the proxy is selected among the groups connected to either the local or the remote router
	QUICK_VALIANT_LOCAL,        // A variant of Valiant for Dragonflies where the proxy is selected among the groups connected to the local router
	QUICK_VALIANT_REMOTE,       // A variant of Valiant for Dragonflies where the proxy is selected among the groups connected to the remote router
	QUICK_VALIANT_PRIVATE,		// A variant of Valiant for Dragonflies where the proxy is selected based on the source so that no other local processor uses that proxy. If param_p == param_h each endpoint has a dedicated uplink/proxy
	QUICK_VALIANT_QUASIPRIVATE,	// A variant of Valiant for Dragonflies where the proxy is selected based on the source so that only other two local processors can use that proxy. If param_p == param_h each endpoint has two uplinks/proxies that share with the previous and next processors
	SPANNING_TREE_ROUTING		// Uses a spanning tree in each VC to avoid deadlock.
} routing_t;

/**
 * Policy to fill out the cam tables
 */
typedef enum cam_policy_t {
    CAM_SP,
    CAM_ECMP,
    CAM_KSP,
    CAM_LLSKR,
    CAM_ALLPATH
} cam_policy_t;

/**
 * Policy to fill out the cam tables
 */
typedef enum cam_ports_t {
    CAM_RND,
    CAM_RR,
    CAM_ADAPTIVE
} cam_ports_t;

typedef enum vc_inj_t {
    VC_INJ_ZERO,
    VC_INJ_DIST,
    VC_INJ_DIST_RND
} vc_inj_t;


/**
* Definition of all request port modes.
*/
typedef enum req_mode_t {
	// Variants for BUBBLE
ONE_OR_MORE_REQUIRED,
	BUBBLE_OBLIVIOUS_REQ, DOUBLE_OBLIVIOUS_REQ,
	ARBITRARY_REQ, ICUBE_REQ,
TWO_OR_MORE_REQUIRED,
	BUBBLE_ADAPTIVE_RANDOM_REQ, BUBBLE_ADAPTIVE_SHORTEST_REQ, BUBBLE_ADAPTIVE_SMART_REQ, DOUBLE_ADAPTIVE_REQ, BIMODAL_REQ,
	// Variants for DALLY
	DALLY_TRC_REQ, DALLY_BASIC_REQ, DALLY_IMPROVED_REQ,
THREE_OR_MORE_REQUIRED,
	DALLY_ADAPTIVE_REQ,
SIX_REQUIRED,
	HEXA_ADAPTIVE_REQ, HEXA_OBLIVIOUS_REQ
} req_mode_t;

/**
* Definition of arbitration mechanism.
*/
typedef enum arb_mode_t {
	ROUNDROBIN_ARB, FIFO_ARB, LONGEST_ARB, HIGHEST_ARB,	RANDOM_ARB, AGE_ARB
} arb_mode_t;

/**
* Definition of injection techniques.
*/
typedef enum inj_mode_t {
	// No prerouting
	SHORTEST_INJ,
	// With prerouting
	DOR_INJ, DOR_SHORTEST_INJ, SHORTEST_PROFITABLE_INJ, LONGEST_PATH_INJ
} inj_mode_t;

/**
* Definition of task placement types for trace driven.
*/
typedef enum placement_t{
	CONSECUTIVE_PLACE, SHUFFLE_PLACE, RANDOM_PLACE, SHIFT_PLACE, ROW_PLACE, COLUMN_PLACE, QUADRANT_PLACE, ICUBE_PLACE, DIAGONAL_PLACE, CIRC_PLACE, FILE_PLACE
} placement_t;

/**
* Definition of the source type for trace driven.
*/
typedef enum source_t{
	NO_SOURCE, INDEPENDENT_SOURCE, OTHER_SOURCE
} source_t;

/**
* Definition of all the parameter types accepted by the simulator.
*/
typedef enum parameters_t{
PAR_BGL,
PAR_PLEN,
PAR_PATTERN,
PAR_LOAD,
PAR_TOPO,
PAR_X,
PAR_Y,
PAR_Z,
PAR_VC,
PAR_ROUTING,
PAR_REQ,
PAR_BUB,
PAR_ARB,
PAR_CONS,
PAR_PLEV,
PAR_PHEAD,
PAR_BHEAD,
PAR_RSEED,
PAR_TQL,
PAR_IQL,
PAR_OUT,
PAR_TRCFILE,
PAR_PINT,
PAR_NWAYS,
PAR_NCHAN,
PAR_NINJ,
PAR_TOUP,
PAR_TOLOW,
PAR_EXTRACT,
PAR_MON,
PAR_INJ,
PAR_IPR,
PAR_DROP,
PAR_LBR,
PAR_PARINJ,
PAR_SHOTM,
PAR_SHOTSIZE,
PAR_UPDATE,
PAR_GCC,
PAR_SKXY,
PAR_SKXZ,
PAR_SKYX,
PAR_SKYZ,
PAR_SKZX,
PAR_SKZY,
PAR_WARMUP,
PAR_CONVP,
PAR_CONVT,
PAR_CONVM,
PAR_NBATCHS,
PAR_BATCHLEN,
PAR_BATCHMIN,
PAR_PLACE,
PAR_LMES,
PAR_LMESR,
PAR_TRIGRATE,
PAR_TRIG,
PAR_FAULTS,
PAR_BW,
PAR_CPUNITS,
PAR_CAM,
PAR_VCINJ,
PAR_SIMICS_REL1,
PAR_SIMICS_REL2,
PAR_SIMICS_SER,
PAR_SIMICS_WAIT
} parameters_t;

// Some declarations.
void * alloc(long);
void abort_sim(char *mes);
void panic(char *mes);

#endif /* _misc */


