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

//#include "constants.h"
//#include <errno.h>
//#include <stdio.h>
//#include <stdlib.h>

/**
 * Choose a random number in [ 0, m ).
 *
 * @param m maximum.
 * @return A random number.
 */
//#define ztm(m) (long) (m * ( (1.0*rand() ) / (RAND_MAX+1.0)))
#define ztm(m) (rand()%m)

/**
 * Return the absolute value
 *
 * @param m The number.
 * @return The absolute value of m.
 */
//#define abs(m) ((m<0) ? (-m) : (m))

/**
 * The sign of a numeric value.
 *
 * For zero, the return value is 1
 */
#define sign(x) (((x)<0 )? -1 : 1)

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
 *
 *
 *
 */
#ifndef iceil
#define iceil(x,y) (x/y+(x%y!=0))
#endif

#ifndef a_better_than_b
#define a_better_than_b(a,b) (a>-1 && (a<b || b<0))
#endif

#ifndef a_con_le_b
#define a_con_le_b(a,b) (a>-1 && (a<=b || b<0))
#endif

#ifndef a_con
#define a_con(a) (a>-1)
#endif

#ifndef a_eq_b_con
#define a_eq_b_con(a,b) (a>-1 && a==b)
#endif


/**
 * Definition of boolean values.
 */
typedef enum bool_t {
    FALSE = 0, TRUE = 1
} bool_t;

/**
 * Definition of all accepted topologies.
 */
typedef enum topo_t {
    FICONN, GDCFICONN, KNKSTAR, HCNBCN, DPILLAR, BCUBE, SWCUBE, SERVERCENTRIC,
    FATTREE, THINTREE, GTREE, JELLYFISH, EXATORUS, EXATREE, NESTGHC, NESTTREE,
    EUROEXA_BASE, EUROEXA_SINGLE, EUROEXA_RND, EUROEXA_MULTI, EUROEXA_RS, EUROEXA_TL,
    SWITCHCENTRIC,
    TORUS, MESH, DIRECT,
    DRAGONFLY_ABSOLUTE, DRAGONFLY_RELATIVE, DRAGONFLY_CIRCULANT, DRAGONFLY_NAUTILUS, DRAGONFLY_HELIX, DRAGONFLY_OTHER,
    MEGAFLY, SLIMFLY
    } topo_t;

typedef enum routing_t {
    DPILLAR_SINGLE_PATH_ROUTING,
    DPILLAR_RANDOM_DIRECTION_SP,
    DPILLAR_SHORTER_DIRECTION_SP,
    DPILLAR_MULTI_PATH_ROUTING,
    DPILLAR_RANDOM_DIRECTION_MP,
    DPILLAR_SHORTER_DIRECTION_MP,
    DPILLAR_MINIMAL_ROUTING,
    JELLYFISH_SHORTEST_PATH_ROUTING,
    JELLYFISH_K_SHORTEST_PATHS_ROUTING,
    JELLYFISH_ECMP_ROUTING,
    JELLYFISH_LLSKR_ROUTING,
    GDCFICONN_DIMENSIONAL,
    GDCFICONN_PROXY,
    HCNBCN_FDIM,
    HCNBCN_NEWFDIM,
    HCNBCN_BDIM,
    HCNBCN_NEWBDIM,
    TREE_STATIC_ROUTING,
    TREE_RND_ROUTING,
    TREE_RR_ROUTING,
    DRAGONFLY_MINIMUM,
    DRAGONFLY_VALIANT,
    DRAGONFLY_QUICK_VALIANT_PRIVATE,          // NUEVO
    DRAGONFLY_QUICK_VALIANT_QUASIPRIVATE,     // NUEVO
    DRAGONFLY_QUICK_VALIANT_LOCAL,            // NUEVO
    DRAGONFLY_QUICK_VALIANT_REMOTE,           // NUEVO
    DRAGONFLY_QUICK_VALIANT_DUAL,             // NUEVO
    MEGAFLY_MINIMUM,
    MEGAFLY_VALIANT,
    SLIMFLY_MINIMUM,
    SLIMFLY_VALIANT,
    EUROEXA_TORUS_ROUTING,
    EUROEXA_ETH_ROUTING,
    EUROEXA_SHORTEST_ROUTING,
    EUROEXA_RANDOM_ROUTING
} routing_t;

/**
 * Classes of traffic sources.
 */
typedef enum tpattern_t {
    BARRIER,
    BCAST,
    REDUCE,
    ALLREDUCE,
    GATHER,
    SCATTER,
    ALLGATHER,
    ALL2ALL,	// All-to-all
    ALL2ONE,     // All-to-one
    ALL2ONERND,
    ONE2ALL,
    ONE2ALLRND,
    MANYALL2ALL,
    MANYALL2ALLRND,
    BUTTERFLY,
    BINARYTREE,
    INVERSEBINARYTREE,
    HOTREGION,
    HOTSPOT,
    RANDOM,
    SHIFT,
    SHIFT_INCREMENTAL,
    SHIFT_RANDOM,
    BISECTION,
    PTP,
    MESH2DWOC,
    MESH2DWC,
    MESH3DWOC,
    MESH3DWC,
    TORUS2DWOC,
    TORUS2DWC,
    TORUS3DWOC,
    TORUS3DWC,
    WATERFALL,
    GUPS,
    NBODIES,
    FILE_PATTERN,
    MAPREDUCE,
    RANDOMAPP,
    RANDOMAPPDCN,
    STORAGEAPP,
    MARKOVAPP
} tpattern_t;

typedef enum placement_t{
    SEQUENTIAL_PLC,
    RANDOM_PLC,
    PATTERN_PLC,
    FILE_PLC
} placement_t;

typedef enum applications_t{
    NONE_APP,
    AUTO_APP,
    FILE_APP
} applications_t;

typedef enum scheduling_t{
    FCFS
} scheduling_t;

typedef enum allocation_t{
    SEQUENTIAL_ALLOC,
    RANDOM_ALLOC,
    JELLYFISH_SPREAD_ALLOC,
    JELLYFISH_RANDOM_ALLOC,
    JELLYFISH_CONTIGUITY_ALLOC,
    JELLYFISH_CONTIGUITY_IF_ALLOC,
    JELLYFISH_CONTIGUITY_IF2_ALLOC,
    JELLYFISH_LOCALITY_ALLOC,
    JELLYFISH_LOCALITY2_ALLOC,
    JELLYFISH_QUASICONTIGUITY_ALLOC
} allocation_t;

typedef enum mapping_t{
    CONSECUTIVE_MAP,
    RANDOM_MAP,
} mapping_t;

typedef enum arrival_t{
    INSTANTANEOUS_ARRIVAL,
    POISSON_ARRIVAL
} arrival_t;

typedef enum rt_mode_t{
    STATIC,
    DYNAMIC_ELECTRIC_FAST,
    DYNAMIC_ELECTRIC_ACCURATE,
    DYNAMIC_PHOTONIC
} rt_mode_t;

typedef enum storage_t{
    RND_STG,
    LOCAL_STG,
    CACHE_STG
} storage_t;

typedef enum memory_storage_access_t{
    RND_MEM_STG_ACCESS,
//    SUCCESS_MEM_STG_ACCESS,
//    FAILURE_MEM_STG_ACCESS
} memory_storage_access_t;

typedef enum data_access_mode_t{
    RND_DATA_ACCESS_MODE,
    CONSECUTIVE_DATA_ACCESS_MODE,
    CACHED_DATA_ACCESS_MODE,
    SAN_DATA_ACCESS_MODE
} data_access_mode_t;

typedef enum stg_nodes_access_mode_t{
    RND_STG_NODES_ACCESS_MODE,
} stg_nodes_access_mode_t;

typedef enum channel_assignment_policy_t{
    STATIC_CHANNEL_ASSIGN,
    ADAPTIVE_CHANNEL_ASSIGN
} channel_assignment_policy_t;

typedef enum lambda_assignment_policy_t{
    STATIC_LAMBDA_ASSIGN,
    ADAPTIVE_LAMBDA_ASSIGN
} lambda_assignment_policy_t;

typedef enum traffic_priority_t{
    FTP_TRAFFIC_PRIORITY,
    TTP_TRAFFIC_PRIORITY,
    STP_TRAFFIC_PRIORITY,
    LITERAL_END
} traffic_priority_t;

#ifndef MAX_TRAFFIC_PARAMS
#define MAX_TRAFFIC_PARAMS 12
#endif

#ifndef MAX_TOPO_PARAMS
#define MAX_TOPO_PARAMS 74
#endif

#ifndef MAX_ROUTING_PARAMS
#define MAX_ROUTING_PARAMS 8
#endif

#ifndef MAX_PLACEMENT_PARAMS
#define MAX_PLACEMENT_PARAMS 8
#endif

#ifndef MAX_SCHEDULING_PARAMS
#define MAX_SCHEDULING_PARAMS 8
#endif

#ifndef MAX_ALLOCATION_PARAMS
#define MAX_ALLOCATION_PARAMS 8
#endif

//AE: I think these values might be too low to be safe.
#ifndef UPPER_PATH_LENGTH
#define UPPER_PATH_LENGTH 1000
#endif

#ifndef UPPER_SERVER_HOPS
#define UPPER_SERVER_HOPS 1000
#endif

//#define MEASURE_ROUTING_TIME

#ifndef CLOCK_MODE
//#define CLOCK_MODE CLOCK_REALTIME
#define CLOCK_MODE CLOCK_MONOTONIC
//#define CLOCK_MODE CLOCK_PROCESS_CPUTIME_ID
#endif // CLOCK_MODE

#endif /* _misc */
