/**
* @file
* @brief	Gets the configuration of the Simulation.
*
* First take default values for the simulation.
* Second take the values from the file 'fsin.conf'.
* Finally take the command line arguments.

FSIN Functional Simulator of Interconnection Networks
Copyright (2003-2011) J. Miguel-Alonso, A. Gonzalez, J. Navaridas

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "globals.h"
#include "literal.h"
#include "misc.h"

/** The Configuration file. */
// #ifndef DEFAULT_CONF_FILE
#define DEFAULT_CONF_FILE "fsin.conf"
// #endif // DEFAULT_CONF_FILE

double bg_load;

// some declarations.
static void set_default_conf(void);
static void get_option(char *);
static void get_conf_file(char *);
static void verify_conf(void);

/**
 * Default parameter names are specified here.
 * @see literal.c
 */
static literal_t options_l[] = {
    {PAR_BGL, "background_load"}, /* load for background traffic*/
    {PAR_BGL, "background"},      /* load for background traffic*/
    {PAR_BGL, "bg_load"},
    {PAR_BGL, "bg"},
    {PAR_PLEN, "plength"},      /* Packet length */
    {PAR_PATTERN, "tpattern"},  /* Traffic pattern */
    {PAR_LOAD, "load"},         /* Load */
    {PAR_TOPO, "topo"},         /* Topology */
    {PAR_X, "nodes_x"},         /* Nodes per dimension: X. */
    {PAR_Y, "nodes_y"},         /* Nodes per dimension: Y. */
    {PAR_Z, "nodes_z"},         /* Nodes per dimension: Z. */
    {PAR_VC, "vc"},             /* VC management */
    {PAR_ROUTING, "routing"},   /* Oblivious routing */
    {PAR_REQ, "rmode"},         /* Request mode */
    {PAR_BUB, "bsize"},         /* Bubble size */
    {PAR_BUB, "bub"},           /* Bubble size */
    {PAR_ARB, "amode"},         /* Arbitration mode */
    {PAR_CONS, "cmode"},        /* Consumption mode */
    {PAR_PLEV, "plevel"},       /* Print level */
    {PAR_PHEAD, "pheaders"},    /* Partial headers */
    {PAR_BHEAD, "bheaders"},    /* Batch headers */
    {PAR_RSEED, "rnds"},        /* Random seed */
    {PAR_RSEED, "rseed"},       /* Random seed */
    {PAR_TQL, "tql"},           /* Transit queue length */
    {PAR_IQL, "iql"},           /* Injection queue length */
    {PAR_OUT, "output"},        /* Path to the output files. */
    {PAR_TRCFILE, "tracefile"}, /* the path to the trace file */
    {PAR_PINT, "pinterval"}, /* Interval to calculate intermediate results and
                                print partial info */
    {PAR_NWAYS, "nways"}, /* Number of ways 1=unidirectional, 2=bidirectional */
    {PAR_NCHAN, "nchan"}, /* Number of virtual channels */
    {PAR_NINJ, "ninj"},   /* Number of injectors */
    {PAR_TOUP, "timeout_upper_limit"},
    {PAR_TOLOW, "timeout_lower_limit"},
    {PAR_EXTRACT, "extract"}, /* Should the system extract packets at head of
                                 inj. queues when they cannot be injected? */
    {PAR_MON, "monitored"}, /* Define the monitored node, to extract additional
                               data from */
    {PAR_INJ, "imode"},     /* Injection mode */
    {PAR_IPR, "intransit_pr"}, /* Priority to give to packets in transit
                                  (against new traffic) */
    {PAR_IPR,
     "ipr"}, /* Priority to give to packets in transit (against new traffic) */
    {PAR_DROP,
     "drop_packets"}, /* Should the system drop packets when the cannot be
                         injected, or wait for a better moment */
    {PAR_DROP, "drop"},
    {PAR_LBR,
     "bub_to_adap"},  /* Apply bubble restrictions even to adaptive channels */
    {PAR_LBR, "lbr"}, /* Apply bubble restrictions even to adaptive channels */
    {PAR_PARINJ, "par_inj"}, /* Parallel vs. serial injection */
    {PAR_SHOTM, "shotmode"}, /* Run in "shot by shot" mode a.k.a. burst-mode*/
    {PAR_SHOTSIZE, "shotsize"},    /* shot size */
    {PAR_UPDATE, "update_period"}, /* update period to compute-distribute global
                                      buffer utilization */
    {PAR_GCC, "global_cc"},        /* global congestion control */
    {PAR_SKXY, "sk_xy"}, /* skews for twisted torus  -- SEE LATER FOR MORE*/
    {PAR_SKXZ, "sk_xz"},
    {PAR_SKYX, "sk_yx"},
    {PAR_SKYZ, "sk_yz"},
    {PAR_SKZX, "sk_zx"},
    {PAR_SKZY, "sk_zy"},
    {PAR_WARMUP, "warm_up_period"},
    {PAR_CONVP, "conv_period"},
    {PAR_CONVT, "conv_thres"},
    {PAR_CONVM, "max_conv_time"},
    {PAR_NBATCHS, "nsamples"},
    {PAR_NBATCHS, "numshots"},
    {PAR_BATCHLEN, "sampling_period"},
    {PAR_BATCHLEN, "sample_size"},
    {PAR_BATCHMIN, "min_batch_pkt"},
    {PAR_PLACE, "placement"},
    {PAR_LMES, "longmessages"},
    {PAR_LMESR, "lng_msg_ratio"},
    {PAR_TRIGRATE, "trigger_rate"},
    {PAR_TRIG, "triggered"},
    {PAR_FAULTS, "faults"},
    {PAR_BW, "bandwidth"},
    {PAR_BW, "link_bandwidth"},
    {PAR_BW, "bw"},
    {PAR_CPUNITS, "trace_cpu_units"},
    {PAR_CPUNITS, "cpu_units"},
    {PAR_CAM, "cam_policy"},
    {PAR_VCINJ, "vc_inj"},
    {PAR_SIMICS_REL1, "fsin_cycle_relation"},
    {PAR_SIMICS_REL2, "simics_cycle_relation"},
    {PAR_SIMICS_SER, "serv_addr"},
    {PAR_SIMICS_WAIT, "num_wait_periods"},
    {PAR_SIMICS_WAIT, "num_periodos_espera"},
    LITERAL_END};

/**
 * All the traffic patterns are specified here.
 * @see literal.c
 */
literal_t pattern_l[] = {
    {UNIFORM, "uniform"},
    {UNIFORM, "random"},
    {LOCAL, "local"},
    {SEMI, "semi"},
    {TRANSPOSE, "transpose"},
    {DISTRIBUTE, "distribute"},
    {HOTREGION, "hotregion"},
    {TORNADO, "tornado"},
    {RSDIST, "rsdist"},
    {COMPLEMENT, "complement"},
    {BUTTERFLY, "butterfly"},
    {SHUFFLE, "shuffle"},
    {REVERSAL, "reversal"},
    {TRACE, "trace"},
    {POPULATION, "population"},
    {POPULATION, "pop"},
    {HISTOGRAM, "histogram"},
    {HISTOGRAM, "hist"},
    {HOTSPOT, "hotspot"},
    {SHIFT, "shift"},
    {GROUPSHIFT, "groupshift"},
    {ADV, "adversarial"},
    {ADV, "adv"},
    {ADV, "worstcase"}, // For Dragonflies. From 'Technology-Driven,
                        // Highly-Scalable Dragonfly Topology'
    {ADV, "wc"},
    {BISECT, "bisect"},
    {BISECT, "bisection"},
    LITERAL_END};

/**
 * All the possible time units for cpu events in the traces.
 * @see literal.c
 */
literal_t cpu_units_l[] = {{UNIT_MILLISECONDS, "ms"},
                           {UNIT_MILLISECONDS, "milliseconds"},
                           {UNIT_MICROSECONDS, "us"},
                           {UNIT_MICROSECONDS, "microseconds"},
                           {UNIT_NANOSECONDS, "ns"},
                           {UNIT_NANOSECONDS, "nanoseconds"},
                           {UNIT_CYCLES, "cycle"},
                           {UNIT_CYCLES, "cycles"},
                           LITERAL_END};

/**
 * All the allowed topologies are specified here.
 * @see literal.c
 */
literal_t topology_l[] = {
    {MIDIMEW, "midimew"},
    {CIRCULANT, "circulant"},
    {CIRC_PK, "circpk"},
    {TWISTED, "ttorus"},
    {TORUS, "torus"},
    {SPINNAKER, "spin"},
    {SPINNAKER, "spinnaker"},
    {MESH, "mesh"},
    {FATTREE, "fattree"},
    {FATTREE, "fat"},
    {THINTREE, "thintree"},
    {THINTREE, "thin"},
    {SLIMTREE, "slimtree"},
    {SLIMTREE, "slim"},
    {SLIMTREE, "slendertree"},
    {SLIMTREE, "slender"},
    {ICUBE, "tricube"},
    {ICUBE, "icube"},
    {RRG, "rrg"},
    {EXA, "exa"},
    {GDBG, "gdbg"},
    {KAUTZ, "kautz"},
    {DRAGONFLY_RELATIVE, "dragonfly"},
    {DRAGONFLY_ABSOLUTE, "dragonfly-abs"},
    {DRAGONFLY_ABSOLUTE, "dragonfly-absolute"},
    {DRAGONFLY_RELATIVE, "dragonfly-rel"},
    {DRAGONFLY_RELATIVE, "dragonfly-relative"},
    {DRAGONFLY_CIRCULANT, "dragonfly-circ"},
    {DRAGONFLY_CIRCULANT, "dragonfly-circulant"},
    {DRAGONFLY_NAUTILUS, "dragonfly-nau"},
    {DRAGONFLY_NAUTILUS, "dragonfly-nautilus"},
    {DRAGONFLY_RND, "dragonfly-random"},
    {DRAGONFLY_RND, "dragonfly-rnd"},
    {DRAGONFLY_RND, "randomfly"},
    {DRAGONFLY_HELIX, "dragonfly-hel"},
    {DRAGONFLY_HELIX, "dragonfly-helix"},
    {DRAGONFLY_OTHER, "dragonfly-other"}, // What is this variant???
    {MEGAFLY, "megafly"},
    {SLIMFLY, "slimfly"},
    LITERAL_END};

/**
 * All the Virtual channel managements are specified here.
 * @see literal.c
 */
literal_t vc_l[] = {{BUBBLE_MANAGEMENT, "bubble"},
                    {DOUBLE_MANAGEMENT, "double"},
                    {DALLY_MANAGEMENT, "dally"},
                    {TREE_MANAGEMENT, "tree"},
                    {ICUBE_MANAGEMENT, "icube"},
                    {GRAPH_NODE_MANAGEMENT, "graph-node"},
                    {GRAPH_PORT_MANAGEMENT, "graph-port"},
                    {GRAPH_NODE_PORT_MANAGEMENT, "graph-node-port"},
                    {GRAPH_DUMMY_MANAGEMENT, "graph-dummy"},
                    {GRAPH_DUMMY_MANAGEMENT, "dummy"},
                    {GRAPH_INC_HOP_MANAGEMENT, "inc-hop"},
                    {VOQ_MANAGEMENT, "voq"},
                    {SPANNING_TREE_MANAGEMENT, "spanning-tree"},
                    {DF_DALLY_MANAGEMENT, "dragonfly-dally"},
                    {DF_DALLY_MANAGEMENT, "df-dally"},
                    {DF_DALLY_MANAGEMENT, "dally-df"},
                    LITERAL_END};

/**
 * All the routing modes are specified here.
 * @see literal.c
 */
literal_t routing_l[] = {
    {DIMENSION_ORDER_ROUTING, "dim"},
    {DIRECTION_ORDER_ROUTING, "dir"},
    {STATIC_ROUTING, "static"},
    {RANDOM_ROUTING, "rnd"},
    {RANDOM_ROUTING, "random"},
    {SRC_ROUTING, "src"},
    {SRC_ROUTING, "source"},
    {DST_ROUTING, "dst"},
    {DST_ROUTING, "destination"},
    {ARITH_ROUTING, "arith"},
    {ARITH_ROUTING, "arithmetic"},
    {ADAPTIVE_ROUTING, "adaptive"},
    {ICUBE_1M_ROUTING, "1mesh"},
    {ICUBE_4M_ROUTING, "4mesh"},
    {CAM_ROUTING, "cam"},
    {VALIANT, "valiant"},
    {VALIANT, "val"},
    {QUICK_VALIANT_DUAL, "quickvaliant"},
    {QUICK_VALIANT_DUAL, "qval"},
    {QUICK_VALIANT_DUAL, "qvd"}, // Journal's official name
    {QUICK_VALIANT_LOCAL, "quickvaliant-local"},
    {QUICK_VALIANT_LOCAL, "quicklocalvaliant"},
    {QUICK_VALIANT_LOCAL, "qlval"},
    {QUICK_VALIANT_LOCAL, "qvl"}, // Journal's official name
    {QUICK_VALIANT_REMOTE, "quickvaliant-remote"},
    {QUICK_VALIANT_REMOTE, "quickremotevaliant"},
    {QUICK_VALIANT_REMOTE, "qrval"},
    {QUICK_VALIANT_REMOTE, "qvr"}, // Journal's official name
    {QUICK_VALIANT_PRIVATE,
     "quickvaliant-fixed"}, // For backwards compatibility with original name
    {QUICK_VALIANT_PRIVATE,
     "quickfixedvaliant"}, // For backwards compatibility with original name
    {QUICK_VALIANT_PRIVATE,
     "qfval"}, // For backwards compatibility with original name
    {QUICK_VALIANT_PRIVATE, "quickvaliant-private"},
    {QUICK_VALIANT_PRIVATE, "quickprivatevaliant"},
    {QUICK_VALIANT_PRIVATE, "qpval"},
    {QUICK_VALIANT_PRIVATE, "qvp"}, // Journal's official name
    {QUICK_VALIANT_QUASIPRIVATE, "qqval"},
    {QUICK_VALIANT_QUASIPRIVATE, "quasivaliant-fixed"},
    {QUICK_VALIANT_QUASIPRIVATE, "quickquasivaliant"},
    {QUICK_VALIANT_QUASIPRIVATE, "qvq"}, // Journal's official name
    {SPANNING_TREE_ROUTING, "spanning-tree"},
    LITERAL_END};

/**
 * All the cam policies are specified here.
 * @see literal.c
 */
literal_t cam_policy_l[] = {{CAM_SP, "sp"},           {CAM_ECMP, "ecmp"},
                            {CAM_KSP, "ksp"},         {CAM_LLSKR, "llskr"},
                            {CAM_ALLPATH, "allpath"}, LITERAL_END};

/**
 * All the cam port policies are specified here.
 * @see literal.c
 */
literal_t cam_ports_policy_l[] = {
    {CAM_RND, "rnd"}, {CAM_RR, "rr"}, {CAM_ADAPTIVE, "adaptive"}, LITERAL_END};

/**
 * All the VC injection modes for graph management are specified here.
 * @see literal.c
 */
literal_t vc_inj_l[] = {{VC_INJ_ZERO, "zero"},
                        {VC_INJ_DIST, "dist"},
                        {VC_INJ_DIST_RND, "dist-rnd"},
                        LITERAL_END};

/**
 * All the port requesting modes are specified here.
 * @see literal.c
 */
literal_t rmode_l[] = {{BUBBLE_ADAPTIVE_RANDOM_REQ, "random"},
                       {BUBBLE_ADAPTIVE_SHORTEST_REQ, "shortest"},
                       {BUBBLE_ADAPTIVE_SMART_REQ, "smart"},
                       {BUBBLE_OBLIVIOUS_REQ, "oblivious"},
                       {DOUBLE_OBLIVIOUS_REQ, "doubleob"},
                       {DOUBLE_ADAPTIVE_REQ, "doubleadap"},
                       {HEXA_OBLIVIOUS_REQ, "hexaob"},
                       {HEXA_ADAPTIVE_REQ, "hexaadap"},
                       {DALLY_TRC_REQ, "trc"},
                       {DALLY_BASIC_REQ, "basic"},
                       {DALLY_IMPROVED_REQ, "improved"},
                       {DALLY_ADAPTIVE_REQ, "adaptive"},
                       {BIMODAL_REQ, "bimodal"},
                       {ICUBE_REQ, "icube"},
                       {ARBITRARY_REQ, "other"},
                       {ARBITRARY_REQ, "arbitrary"},
                       {ARBITRARY_REQ, "trees"},
                       {ARBITRARY_REQ, "graph"},
                       LITERAL_END};

/**
 * All the arbitration policies are specified here.
 * @see literal.c
 */
literal_t atype_l[] = {{ROUNDROBIN_ARB, "rr"},   {FIFO_ARB, "fifo"},
                       {LONGEST_ARB, "longest"}, {HIGHEST_ARB, "highest"},
                       {RANDOM_ARB, "random"},   {AGE_ARB, "oldest"},
                       {AGE_ARB, "age"},         LITERAL_END};

/**
 * All the consumption modes are specified here.
 * @see literal.c
 */
literal_t ctype_l[] = {
    {SINGLE_CONS, "single"}, {MULTIPLE_CONS, "multiple"}, LITERAL_END};

/**
 * All the injection policies are specified here.
 * @see literal.c
 */
literal_t injmode_l[] = {
    {SHORTEST_INJ, "shortest"},  {DOR_INJ, "dor"},
    {DOR_SHORTEST_INJ, "dsh"},   {SHORTEST_PROFITABLE_INJ, "shp"},
    {LONGEST_PATH_INJ, "lpath"}, LITERAL_END};

/**
 * All the placement strategies are specified here.
 * @see literal.c
 */
literal_t placement_l[] = {{CONSECUTIVE_PLACE, "consecutive"},
                           {SHUFFLE_PLACE, "shuffle"},
                           {RANDOM_PLACE, "random"},
                           {SHIFT_PLACE, "shift"},
                           {COLUMN_PLACE, "column"},
                           {ROW_PLACE, "row"},
                           {QUADRANT_PLACE, "quadrant"},
                           {DIAGONAL_PLACE, "diagonal"},
                           {ICUBE_PLACE, "icube"},
                           {FILE_PLACE, "file"},
                           LITERAL_END};

/**
 * Gets the configuration defined into a file.
 * @param fname The name of the file containing the configuration.
 */
void get_conf_file(char *fname) {
  FILE *fdesc;
  char buffer[1024];
  long len;

  if ((fdesc = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "WARNING: config file %s not found in current directory\n",
            fname);
    return;
  }

  while (fgets(buffer, 1024, fdesc) != NULL)
    if (buffer[0] != '\n' && buffer[0] != '#' && buffer[0] != '\r') {
      len = strlen(buffer);
      if (buffer[len - 2] == '\n' || buffer[len - 2] == '\r')
        buffer[len - 2] = '\0';
      if (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')
        buffer[len - 1] = '\0';
      get_option(buffer);
    }
  fclose(fdesc);
}

/**
 * Get the configuration for the simulation.
 *
 * First take default values for the simulation.
 * Second take the values from the file 'fsin.conf'.
 * Finally take the command line arguments.
 * @param argn The number of arguments.
 * @param args The arguments taken from the command line.
 */
void get_conf(long argn, char **args) {
  long i;

  set_default_conf();
  get_conf_file(DEFAULT_CONF_FILE);
  for (i = 0; i < argn; ++i)
    get_option(args[i]);
  verify_conf();
}

/**
 * Gets an option & its value.
 *
 * Put the value to the FSIN variable.
 * @param option The string which contains an option=value
 */
void get_option(char *option) {
  int opt;
  long aux;

  char *name;
  char *value;
  char *param;
  char *sep = " _";
  char message[100];

  name = strtok(option, "=");
  if (!literal_value(options_l, name, (int *)&opt)) {
    sprintf(message, "get_option: Unknown option %s", name);
    panic(message);
  }
  value = strtok(NULL, "=");
  switch (opt) {
  case PAR_BGL:
    sscanf(value, "%lf", &bg_load);
    break;
  case PAR_PLEN:
    param = strtok(value, sep);
    if (param)
      pkt_len = atoi(param);
    param = strtok(NULL, sep);
    if (param)
      phit_len = atoi(param);
    break;
  case PAR_PATTERN:
    param = strtok(value, sep);
    if (!literal_value(pattern_l, param, (int *)&pattern)) {
      sprintf(message, "get_conf: Unknown traffic pattern: %s", param);
      panic(message);
    }
    if (pattern == SHIFT) {
      param = strtok(NULL, sep);
      if (param)
        stride = atoi(param);
    } else if (pattern == GROUPSHIFT) {
      param = strtok(NULL, sep);
      if (param)
        group_size = atoi(param);
      param = strtok(NULL, sep);
      if (param)
        stride = atoi(param);
    }
    break;
  case PAR_LOAD:
    sscanf(value, "%lf", &load);
    break;
  case PAR_TOPO:
    param = strtok(value, sep);
    if (!literal_value(topology_l, param, (int *)&topo)) {
      sprintf(message, "get_conf: Unknown topology: %s", param);
      panic(message);
    }
    if (topo < DIRECT) {
      param = strtok(NULL, sep);
      if (param) {
        nodes_x = atoi(param);
        ndim = 1;
      }
      param = strtok(NULL, sep);
      if (param) {
        nodes_y = atoi(param);
        ndim = 2;
      }
      param = strtok(NULL, sep);
      if (param) {
        nodes_z = atoi(param);
        ndim = 3;
      }
    } else if (topo == FATTREE) {
      param = strtok(NULL, sep);
      if (param)
        stDown = stUp = atoi(param);
      param = strtok(NULL, sep);
      if (param)
        nstages = atoi(param);
      radix = stDown + stUp;
    } else if (topo == SLIMTREE || topo == THINTREE) {
      param = strtok(NULL, sep);
      if (param)
        stDown = atoi(param);
      param = strtok(NULL, sep);
      if (param)
        stUp = atoi(param);
      param = strtok(NULL, sep);
      if (param)
        nstages = atoi(param);
      radix = stDown + stUp;
    } else if (topo == DRAGONFLY_ABSOLUTE || topo == DRAGONFLY_RELATIVE ||
               topo == DRAGONFLY_CIRCULANT || topo == DRAGONFLY_NAUTILUS ||
               topo == DRAGONFLY_RND || topo == DRAGONFLY_HELIX ||
               topo == DRAGONFLY_OTHER) {
      param = strtok(NULL, sep);
      if (param)
        stDown = param_p = atoi(param); // number of servers per switch
      else
        panic("3 parameters are needed for constructing a dragonfly topology: "
              "p,a,h");

      param = strtok(NULL, sep);
      if (param)
        param_a = atoi(param); // number of switches per group
      else
        panic("3 parameters are needed for constructing a dragonfly topology");

      param = strtok(NULL, sep);
      if (param)
        param_h = atoi(param); // number of uplinks per switch
      else
        panic("3 parameters are needed for constructing a dragonfly topology");

      radix = param_h + param_p + param_a - 1;
    } else if (topo == MEGAFLY) {
      param = strtok(NULL, sep);
      if (param)
        radix = atoi(param); // number of servers per switch
      else
        panic("3 parameters are needed for constructing a dragonfly topology: "
              "p,a,h");
      stDown = param_p = radix / 2; // number of servers per local switch
      param_a = radix;              // number of switches per group
      param_h = param_p;            // number of uplinks per global switch

    } else if (topo == SLIMFLY) {
      param = strtok(NULL, sep);
      if (param)
        param_q = atoi(param); // number of servers per switch
      else
        panic("3 parameters are needed for constructing a dragonfly topology: "
              "p,a,h");
      // stDown = param_p = radix/2; //number of servers per local switch
      // param_a = radix; //number of switches per group
      // param_h = param_p; //number of uplinks per global switch
      int out = 0;
      // param_delta_opt[3] = {-1,0,1};
      int param_l;
      // check wether q is a prime of the form q = 4w+delta
      for (int j = 0; j < 100; j++) {
        for (int i = 0; i < 3; i++) {
          if (param_q == (4 * j + param_delta_opt[i])) {
            out = 1;
            param_delta_final = param_delta_opt[i];
            param_l = j;
            // printf("param_delta_final: %d\n", param_delta_final);
            break;
          }
        }
        if (out)
          break;
      }
      param_k = (3 * param_q - param_delta_final) / 2; // links a otros routers
      switches = 2 * param_q * param_q;                // number of switches
      param_p = (param_k + 1) / 2; // number of servers per switch
      servers = param_p * switches;
      param_a = param_q; // number of switches per group
      param_h = param_q; // number of uplinks per switch
      // printf("q: %d \n", param_q);
      // printf("k: %d \n", param_k);
      // printf("switches: %d \n", switches);
      // printf("p: %d \n", param_p);
      //
      // printf("servers: %d \n", servers);
      // Calculate some useful values from the parameters
      intra_ports = ((param_q - param_delta_final) / 2);
      grps = param_q * 2;
      param_tam_gal = (param_q - param_delta_final) / 2;
      radix = param_p + param_tam_gal + param_h;
      int ports = (param_k + param_p) * switches + servers;

      // calcular el Galois Field con q
      int param_field[param_q];
      for (int i = 0; i < param_q; i++) {
        param_field[i] = i;
      }

      // mirar como buscar generadores de galois fields eficientemente
      int contador[param_q - 1];
      int param_gen;

      for (int i = 2; i < param_q; i++) {
        param_gen = i;

        for (int i2 = 0; i2 < param_q - 1; i2++)
          contador[i2] = 0;

        for (int j = 0; j < param_q - 1; j++) {
          int prueba_gen = (int)pow(param_gen, j);

          for (int k = 1; k < param_q; k++) {

            if ((prueba_gen % param_q) == param_field[k]) {
              contador[k - 1] = 1;
              break;
            }
          }
        }
        int es_generador = 1;
        for (int i3 = 0; i3 < param_q - 1; i3++)
          if (contador[i3] == 0) {
            es_generador = 0;
            break;
          }
        if (es_generador)
          break;
      }

      // int *param_x, *param_x2;
      param_x = malloc(param_tam_gal * sizeof(int));
      param_x2 = malloc(param_tam_gal * sizeof(int));

      if (param_q % 4 == 1) {
        for (int i = 0; i < param_q - 1; i++) {
          if (i % 2 == 0)
            param_x[i / 2] = mod(((int)pow(param_gen, i)), param_q);
          if (i % 2 != 0)
            param_x2[i / 2] = mod(((int)pow(param_gen, i)), param_q);
          // printf("param_x: %d", param_x[i/2]);
          // printf("param_x2: %d", param_x2[i/2]);
        }
      } else if (param_q % 4 == 3) {
        for (int i = 0; i < (2 * param_l) - 1; i += 2) {
          param_x[i / 2] = mod(((int)pow(param_gen, i)), param_q);
          param_x[param_l + i / 2] =
              mod(((int)pow(param_gen, 2 * param_l - 1 + i)), param_q);
          // printf("param_x en %d: %d**%d \n", i/2, param_gen, i);
          // printf("param_x en %d: %d**%d\n", i/2, param_gen, 2*param_l-1+i);

          param_x2[i / 2] = mod(((int)pow(param_gen, i + 1)), param_q);
          param_x2[param_l + i / 2] =
              mod(((int)pow(param_gen, 2 * param_l + i)), param_q);
          // printf("param_x2 en %d: %d**%d\n", i/2, param_gen, i+1);
          // printf("param_x2 en %d: %d**%d\n", i/2, param_gen, 2*param_l+i);
        }
      }

    } else if (topo == ICUBE) {
      param = strtok(NULL, sep);
      if (param) {
        stDown = atoi(param);
      }
      param = strtok(NULL, sep);
      if (param) {
        links_per_direction = atoi(param);
      }
      param = strtok(NULL, sep);
      if (param) {
        nodes_x = atoi(param);
        ndim = 1;
      }

      param = strtok(NULL, sep);
      if (param) {
        nodes_y = atoi(param);
        ndim = 2;
      } else
        nodes_y = 1;

      param = strtok(NULL, sep);
      if (param) {
        nodes_z = atoi(param);
        ndim = 3;
      } else
        nodes_z = 1;
    } else if (topo == RRG || topo == EXA || topo == GDBG || topo == KAUTZ) {
      if (topo == RRG || topo == EXA) {
        param = strtok(NULL, sep);
        if (param)
          sscanf(param, "%s", topology_filename);
        else
          panic("A file with the topology must be provided");
      }
      param = strtok(NULL, sep);
      if (param)
        nswitches = atoi(param);
      else
        panic("The number of switches must be provided");
      param = strtok(NULL, sep);
      if (param) {
        radix = atoi(param);
      } else
        panic("The number of ports of the switches must be provided");
      param = strtok(NULL, sep);
      if (param) {
        stUp = atoi(param);    // ports used for connectivity
        stDown = radix - stUp; // computing nodes per switch
      } else
        panic("Number of ports used for connectivity must be provided");
      param = strtok(NULL, sep);
      if (param) {
        nnics = atoi(param);
      } else {
        nnics = 1;
      }
      nways = 1;
    }
    break;
  case PAR_X:
  case PAR_Y:
  case PAR_Z:
    panic("The Correct Syntax is now "
          "\"topo=<direct_topology>_<nodes_x>_<nodes_y>_<nodes_z>\" ");
    break;
  case PAR_VC:
    if (!literal_value(vc_l, value, (int *)&vc_management)) {
      sprintf(message, "get_conf: Unknown VC management: %s", value);
      panic(message);
    }
    break;
  case PAR_ROUTING:
    if (!literal_value(routing_l, value, (int *)&routing)) {
      sprintf(message, "get_conf: Unknown routing: %s", value);
      panic(message);
    }
    break;
  case PAR_REQ:
    if (!literal_value(rmode_l, value, (int *)&req_mode)) {
      sprintf(message, "get_conf: Unknown request policy: %s", value);
      panic(message);
    }
    break;
  case PAR_BUB:
    param = strtok(value, sep);
    bub_x = bub_y = bub_z = 2;
    if (param)
      bub_y = bub_z = bub_x = atoi(param);
    param = strtok(NULL, sep);
    if (param)
      bub_z = bub_y = atoi(param);
    param = strtok(NULL, sep);
    if (param)
      bub_z = atoi(param);
    break;
  case PAR_ARB:
    if (!literal_value(atype_l, value, (int *)&arb_mode)) {
      sprintf(message, "get_conf: Unknown arbitration policy: %s", value);
      panic(message);
    }
    break;
  case PAR_CONS:
    if (!literal_value(ctype_l, value, (int *)&cons_mode)) {
      sprintf(message, "get_conf: Unknown consumption mode: %s", value);
      panic(message);
    }
    break;
  case PAR_PLEV:
    sscanf(value, "%ld", &plevel);
    break;
  case PAR_PHEAD:
    sscanf(value, "%ld", &pheaders);
    if (pheaders > 2047)
      panic("get_conf: Invalid partial header value");
    break;
  case PAR_BHEAD:
    sscanf(value, "%ld", &bheaders);
    if (pheaders > 8191)
      panic("get_conf: Invalid batch header value");
    break;
  case PAR_RSEED:
    sscanf(value, "%ld", &r_seed);
    break;
  case PAR_TQL:
    sscanf(value, "%ld", &buffer_cap);
    break;
  case PAR_IQL:
    sscanf(value, "%ld", &binj_cap);
    break;
  case PAR_OUT:
    sscanf(value, "%s", file);
    break;
  case PAR_TRCFILE:
    sscanf(value, "%s", trcfile);
    break;
  case PAR_PINT:
    sscanf(value, "%" SCAN_CLOCK, &pinterval);
    break;
  case PAR_NWAYS:
    sscanf(value, "%ld", &nways);
    break;
  case PAR_NCHAN:
    sscanf(value, "%ld", &nchan);
    break;
  case PAR_NINJ:
    sscanf(value, "%ld", &ninj);
    break;
  case PAR_TOUP:
    sscanf(value, "%" SCAN_CLOCK, &timeout_upper_limit);
    break;
  case PAR_TOLOW:
    sscanf(value, "%" SCAN_CLOCK, &timeout_lower_limit);
    break;
  case PAR_EXTRACT:
    sscanf(value, "%ld", &aux);
    if (aux)
      extract = B_TRUE;
    else
      extract = B_FALSE;
    break;
  case PAR_MON:
    sscanf(value, "%ld", &monitored);
    break;
  case PAR_INJ:
    if (!literal_value(injmode_l, value, (int *)&inj_mode)) {
      sprintf(message, "get_conf: Unknown injection mode: %s", value);
      panic(message);
    }
    break;
  case PAR_IPR:
    sscanf(value, "%lf", &intransit_pr);
    break;
  case PAR_DROP:
    sscanf(value, "%ld", &aux);
    if (aux)
      drop_packets = B_TRUE;
    else
      drop_packets = B_FALSE;
    break;
  case PAR_LBR:
    sscanf(value, "%ld", &bub_adap[1]);
    break;
  case PAR_PARINJ:
    sscanf(value, "%ld", &aux);
    if (aux)
      parallel_injection = B_TRUE;
    else
      parallel_injection = B_FALSE;
    break;
  case PAR_SHOTM:
    sscanf(value, "%ld", &aux);
    if (aux)
      shotmode = B_TRUE;
    else
      shotmode = B_FALSE;
    break;
  case PAR_SHOTSIZE:
    sscanf(value, "%ld", &shotsize);
    break;
  case PAR_UPDATE:
    sscanf(value, "%" SCAN_CLOCK, &update_period);
    break;
  case PAR_GCC:
    sscanf(value, "%lf", &global_cc);
    break;
  case PAR_SKXY:
    sscanf(value, "%ld", &sk_xy);
    break;
  case PAR_SKXZ:
    sscanf(value, "%ld", &sk_xz);
    break;
  case PAR_SKYX:
    sscanf(value, "%ld", &sk_yx);
    break;
  case PAR_SKYZ:
    sscanf(value, "%ld", &sk_yz);
    break;
  case PAR_SKZX:
    sscanf(value, "%ld", &sk_zx);
    break;
  case PAR_SKZY:
    sscanf(value, "%ld", &sk_zy);
    break;
  case PAR_WARMUP:
    sscanf(value, "%" SCAN_CLOCK, &warm_up_period);
    break;
  case PAR_CONVP:
    sscanf(value, "%" SCAN_CLOCK, &conv_period);
    break;
  case PAR_CONVT:
    sscanf(value, "%lf", &threshold);
    break;
  case PAR_CONVM:
    sscanf(value, "%" SCAN_CLOCK, &max_conv_time);
    break;
  case PAR_NBATCHS:
    sscanf(value, "%ld", &samples);
    break;
  case PAR_BATCHLEN:
    sscanf(value, "%" SCAN_CLOCK, &batch_time);
    break;
  case PAR_BATCHMIN:
    sscanf(value, "%ld", &min_batch_size);
    break;
  case PAR_PLACE:
    param = strtok(value, sep);
    if (!literal_value(placement_l, param, (int *)&placement)) {
      sprintf(message, "get_conf: Unknown placement policy: %s", param);
      panic(message);
    }
    if (placement == SHIFT_PLACE) {
      param = strtok(NULL, sep);
      if (param)
        shift_plc = atoi(param);
      else
        shift_plc = 0;
    }
    if (placement == ICUBE_PLACE) {
      param = strtok(NULL, sep);
      if (param)
        pnodes_x = atoi(param);
      else
        pnodes_x = 1;
      param = strtok(NULL, sep);
      if (param)
        pnodes_y = atoi(param);
      else
        pnodes_y = 1;
      param = strtok(NULL, sep);
      if (param)
        pnodes_z = atoi(param);
      else
        pnodes_z = 1;
    }
    if (placement == FILE_PLACE) {
      param = strtok(NULL, sep);
      if (param)
        strcpy(placefile, param);
      else
        panic("placement from file requires a placement file");
    }
    param = strtok(NULL, sep);
    if (param)
      trace_nodes = atoi(param);
    else
      trace_nodes = 0;
    param = strtok(NULL, sep);
    if (param)
      trace_instances = atoi(param);
    else
      trace_instances = 0;
    break;
  case PAR_LMES:
#if (BIMODAL_SUPPORT != 0)
    sscanf(value, "%ld", &msglength);
#endif /* BIMODAL */
    break;
  case PAR_LMESR:
#if (BIMODAL_SUPPORT != 0)
    sscanf(value, "%lf", &lm_percent);
#endif /* BIMODAL */
    break;
  case PAR_TRIGRATE:
    sscanf(value, "%lf", &trigger_rate);
    break;
  case PAR_TRIG:
    param = strtok(value, sep);
    trigger_max = trigger_min = 0;
    if (param)
      trigger_max = trigger_min = atoi(param);
    param = strtok(NULL, sep);
    if (param)
      trigger_min = atoi(param);
    if (trigger_min > trigger_max) { // Just in case
      long aux;
      aux = trigger_min;
      trigger_min = trigger_max;
      trigger_max = aux;
    }
    break;
  case PAR_FAULTS:
    sscanf(value, "%ld", &faults);
    break;
  case PAR_BW:
    sscanf(value, "%ld", &link_bw);
    break;
  case PAR_CPUNITS:
    if (!literal_value(cpu_units_l, value, (int *)&cpu_units)) {
      sprintf(message, "get_conf: Invalid CPU units %s", value);
      panic(message);
    }
    break;
  case PAR_CAM:
    param = strtok(value, sep);
    if (!literal_value(cam_policy_l, param, (int *)&cam_policy)) {
      sprintf(message, "get_conf: Unknown CAM policy %s", param);
      panic(message);
    }
    param = strtok(NULL, sep);
    if (param) {
      if (!literal_value(cam_ports_policy_l, param, (int *)&cam_ports)) {
        sprintf(message, "get_conf: Unknown PORTS selection policy: %s", param);
        panic(message);
      }
    }
    param = strtok(NULL, sep);
    if (param)
      cam_policy_params[0] = atoi(param);
    param = strtok(NULL, sep);
    if (param)
      cam_policy_params[1] = atoi(param);
    param = strtok(NULL, sep);
    if (param)
      cam_policy_params[2] = atoi(param);
    break;
  case PAR_VCINJ:
    if (!literal_value(vc_inj_l, value, (int *)&vc_inj)) {
      sprintf(message, "get_conf: Unknown VC injection policy: %s", value);
      panic(message);
    }
    break;

#if (EXECUTION_DRIVEN != 0)
  case PAR_SIMICS_REL1:
    sscanf(value, "%ld", &fsin_cycle_relation);
    break;
  case PAR_SIMICS_REL2:
    sscanf(value, "%ld", &simics_cycle_relation);
    break;
  case PAR_SIMICS_SER:
    sscanf(value, "%ld", &serv_addr);
    break;
  case PAR_SIMICS_WAIT:
    sscanf(value, "%ld", &num_periodos_espera);
    break;
#else
  case PAR_SIMICS_REL1:
  case PAR_SIMICS_REL2:
  case PAR_SIMICS_SER:
  case PAR_SIMICS_WAIT:
    break;
#endif

  default:
    panic("Should not be here in get_option");
  }
}

/**
 * Verifies the simulation configuration.
 *
 * Looks for illegal values of the variables or not allowed combinations.
 */
void verify_conf(void) {
  char mon[128];

  if (pkt_len < 1 || phit_len < 1)
    panic("verify_conf: Illegal packet length");
  if (!(bub_adap[1] <= buffer_cap && bub_x <= buffer_cap &&
        bub_y <= buffer_cap && bub_z <= buffer_cap))
    panic("Illegal bubble size");
  tr_ql = buffer_cap * pkt_len + 1;
  inj_ql = binj_cap * pkt_len + 1;

  if (topo == ICUBE && nways != 2) {
    printf("WARNING: only bidirectional icubes implemented\n");
    printf("         Setting nways to 2!!!\n");
    nways = 2;
  }
  if (topo == CIRC_PK) {
    if (ndim != 2) {
      panic("Only 2D circulant graphs implemented so far!!!");
    }
    a_circ = nodes_x;
    k_circ = nodes_y;

    if (gcd(a_circ, k_circ) != 1)
      panic("Greatest common divisor of a and k is not 1");
    k_inv = inverse(k_circ, a_circ);

    s1 = 1;
    s2 = 2 * k_circ * a_circ - 1;

    nodes_x = 2 * a_circ * a_circ;
    nodes_y = 1; // to calculate correctly y#the total number of nodes
  }

  if (topo == CIRCULANT) {
    if (ndim != 2) {
      panic("Only 2D circulant graphs implemented so far!!!");
    }
    step = nodes_y; // the distance to the second dimension of adjacency
    nodes_y = 1;    // to calculate correctly y#the total number of nodes

    twist = nodes_x % step;
    rows = nodes_x / step;

    if (twist > step / 2) {
      twist = twist - step;
      rows = rows + 1;
    }
  }

  if (topo > CUBE && nways != 1) {
    printf("WARNING: nways has no sense in indirect topologies\n");
    printf("         Setting nways to 1!!!\n");
    nways = 1;
  }

  if ((req_mode == DOUBLE_OBLIVIOUS_REQ && (nchan % ndim))) {
    printf("WARNING: Bubble double oblivious only works for a number of VCs "
           "multiple of ndim\n");
    printf("         Setting nchan to %ld\n", ndim);
    nchan = ndim;
  }

  if ((req_mode == DOUBLE_ADAPTIVE_REQ && (nchan % ndim))) {
    printf("WARNING: CURRENTLY double adaptive only works for a number of VC "
           "multiple of ndim\n");
    printf("         Setting nchan to %ld\n", ndim);
    nchan = ndim;
  }
  if (routing == SPANNING_TREE_ROUTING) {
    vc_management = SPANNING_TREE_MANAGEMENT;
    if (nchan <= 0)
      nchan = 1;
    printf("WARNING: Spanning-tree routing requires spanning-tree management.");
    printf("         Setting VC management to SPANNING_TREE_MANAGEMENT!!!\n");
  }
  if (ndim == 1)
    nodes_y = nodes_z = 1; // Just in case...
  else if (ndim == 2)
    nodes_z = 1;

  // Direct topologies are mesh, torus, ttorus and midimew.
  if (topo < DIRECT) {
    NUMNODES = (nodes_x * nodes_y * nodes_z);
    nprocs = NUMNODES;
    radix = ndim * nways;
    if (placement == SHUFFLE_PLACE) {
      printf(
          "WARNING: SHUFFLE placement only for indirect/hybrid topologies\n");
      printf("         Setting placement to COLUMN placement!!!\n");
      placement = COLUMN_PLACE;
    }
  } else if (topo == FATTREE) {
    nprocs = (long)pow(stDown, nstages);
    NUMNODES = ((long)pow(stDown, nstages - 1) * nstages) + nprocs;
    req_mode = ARBITRARY_REQ;
    if (routing == ADAPTIVE_ROUTING || routing == ARITH_ROUTING)
      vc_management = TREE_MANAGEMENT;

    if (inj_mode != SHORTEST_INJ) {
      printf("WARNING: Injection with pre-routing only implemented for cube "
             "topology\n");
      printf("         Setting imode to SHORTEST!!!\n");
      inj_mode = SHORTEST_INJ;
    }
  }
  // thin tree
  else if (topo == THINTREE) {
    long st, aux;
    aux = (long)pow(stDown, nstages - 1);
    nprocs = aux * stDown; // stDown ^ nstages
    NUMNODES = aux + nprocs;
    for (st = 2; st <= nstages; st++) {
      aux = stUp *
            (aux /
             stDown); // nodes in each stage. :: stUp^N-1*(stDown/stUp)^(N-st)
      NUMNODES += aux;
    }
    req_mode = ARBITRARY_REQ;
    if (routing == ADAPTIVE_ROUTING || routing == ARITH_ROUTING)
      vc_management = TREE_MANAGEMENT;

    if (inj_mode != SHORTEST_INJ) {
      printf("WARNING: Injection with pre-routing only implemented for cube "
             "topology\n");
      printf("         Setting imode to SHORTEST!!!\n");
      inj_mode = SHORTEST_INJ;
    }
  } else if (topo == SLIMTREE) { // slimmed (aka slender) trees
    long st, aux;
    if ((stDown % stUp) != 0) // it also panics if stUp > stDown.
      panic("Slimtree requires stDown to be divisible by stUp");
    nprocs = (long)pow((stDown / stUp), nstages) * stUp * stUp;
    aux = stUp;
    NUMNODES = stUp + nprocs;
    for (st = 1; st < nstages; st++) {
      aux = aux * stDown / stUp; // nodes in each stage.
      NUMNODES += aux;
    }
    req_mode = ARBITRARY_REQ;
    vc_management = TREE_MANAGEMENT;
    if (inj_mode != SHORTEST_INJ) {
      printf("WARNING: Injection with pre-routing only implemented for cube "
             "topology\n");
      printf("         Setting imode to SHORTEST!!!\n");
      inj_mode = SHORTEST_INJ;
    }
  } else if (topo == DRAGONFLY_ABSOLUTE || topo == DRAGONFLY_RELATIVE ||
             topo == DRAGONFLY_CIRCULANT || topo == DRAGONFLY_RND ||
             topo == DRAGONFLY_NAUTILUS || topo == DRAGONFLY_HELIX ||
             topo == DRAGONFLY_OTHER) {
    intra_ports = param_a - 1;
    grps = (param_a * param_h) + 1;
    nprocs = param_p * param_a * grps;
    NUMNODES = nprocs + (param_a * grps);

    req_mode = ARBITRARY_REQ;

    if (routing == DIMENSION_ORDER_ROUTING) // DIM uses static routing
      routing = STATIC_ROUTING;

    if (vc_management ==
        DALLY_MANAGEMENT) // in case Dally for k-ary n-cubes has been selected,
                          // switch to Dally for dragonfly
      vc_management = DF_DALLY_MANAGEMENT;

    if (vc_management != GRAPH_NODE_MANAGEMENT &&
        vc_management != GRAPH_PORT_MANAGEMENT &&
        vc_management != GRAPH_NODE_PORT_MANAGEMENT &&
        vc_management != GRAPH_DUMMY_MANAGEMENT &&
        vc_management != VOQ_MANAGEMENT &&
        vc_management != SPANNING_TREE_MANAGEMENT &&
        vc_management != GRAPH_INC_HOP_MANAGEMENT &&
        vc_management != DF_DALLY_MANAGEMENT) {
      printf("WARNING: Using dragonfly without a VC management\n");
      printf("         Setting vc_management to DUMMY.  Deadlocks are likely "
             "to appear!!!\n");
      vc_management = GRAPH_DUMMY_MANAGEMENT;
    }
    if (inj_mode != SHORTEST_INJ) {
      printf("WARNING: Injection with pre-routing only implemented for cube "
             "topology\n");
      printf("         Setting imode to SHORTEST!!!\n");
      inj_mode = SHORTEST_INJ;
    }
  } else if (topo == MEGAFLY) {
    grps = (param_a / 2) * param_h + 1;
    intra_ports = param_p;
    nprocs = param_p * (param_a / 2) * grps;
    NUMNODES = nprocs + (param_a * grps);
    req_mode = ARBITRARY_REQ;
    if (routing == DIMENSION_ORDER_ROUTING)
      routing = STATIC_ROUTING;

    if (vc_management == DALLY_MANAGEMENT)
      vc_management = DF_DALLY_MANAGEMENT;
  } else if (topo == SLIMFLY) {
    intra_ports = ((param_q - param_delta_final) / 2);
    grps = param_q * 2;
    nprocs = param_p * switches;
    NUMNODES = nprocs + switches;
    req_mode = ARBITRARY_REQ;

  } else if (topo == ICUBE) {
    nprocs = stDown * (nodes_x * nodes_y * nodes_z);
    NUMNODES = (stDown + 1) * (nodes_x * nodes_y * nodes_z);
    radix = stDown + (ndim * 2 * links_per_direction);
    req_mode = ICUBE_REQ;
    vc_management = ICUBE_MANAGEMENT;
    if (inj_mode != SHORTEST_INJ) {
      printf("WARNING: Injection with pre-routing only implemented for cube "
             "topology\n");
      printf("         Setting imode to SHORTEST!!!\n");
      inj_mode = SHORTEST_INJ;
    }
  } else if (topo == RRG || topo == EXA || topo == GDBG || topo == KAUTZ) {
    nprocs = (stDown * nswitches) / nnics;
    NUMNODES = nprocs + nswitches;
    req_mode = ARBITRARY_REQ;
    if (vc_management != GRAPH_NODE_MANAGEMENT &&
        vc_management != GRAPH_PORT_MANAGEMENT &&
        vc_management != GRAPH_NODE_PORT_MANAGEMENT &&
        vc_management != GRAPH_DUMMY_MANAGEMENT &&
        vc_management != VOQ_MANAGEMENT &&
        vc_management != GRAPH_INC_HOP_MANAGEMENT &&
        vc_management != SPANNING_TREE_MANAGEMENT) {
      printf("WARNING: Using a graph topology without VC management\n");
      printf("         Setting vc_management to DUMMY. Deadlocks are likely to "
             "appear!!!\n");
      vc_management = GRAPH_DUMMY_MANAGEMENT;
    }
    if (inj_mode != SHORTEST_INJ) {
      printf("WARNING: Injection with pre-routing only implemented for cube "
             "topology\n");
      printf("         Setting imode to SHORTEST!!!\n");
      inj_mode = SHORTEST_INJ;
    }
  }

  if ((inj_mode >= DOR_INJ) && (ninj < (ndim * nways))) {
    ninj = ndim * nways;
    printf("WARNING: Injection with pre-routing requires more injectors");
    printf("         Setting the number of injectors to %ld!!!\n", ninj);
  }

  if (vc_management == VOQ_MANAGEMENT) {
    if (nchan != radix) {
      printf("WARNING: Virtual Output Queue management selected\n");
      printf("         updating the number of VCs to be the switch radix "
             "(%ld)!!!\n",
             radix);
      nchan = radix;
    }
  }

  n_ports = radix * nchan + ninj + 1;

  if (topo == MIDIMEW) {
    if (ndim != 1)
      panic("Midimew networks allow only one parameter");
    else
      ndim = 2;
  }

  if (topo == TWISTED) {
    if (sk_xy >= nodes_y)
      panic("dtt_neighbor: Skew too large");
    if (sk_xz >= nodes_z)
      panic("dtt_neighbor: Skew too large");
    if (sk_yx >= nodes_x)
      panic("dtt_neighbor: Skew too large");
    if (sk_yz >= nodes_z)
      panic("dtt_neighbor: Skew too large");
    if (sk_zx >= nodes_x)
      panic("dtt_neighbor: Skew too large");
    if (sk_zy >= nodes_y)
      panic("dtt_neighbor: Skew too large");

    // Only skews of two dimensions over the third one are permited this must be
    // checked in the initialization.
    if (sk_xy != 0) {
      if ((sk_xz != 0) || (sk_yx != 0) || (sk_yz != 0) || (sk_zx != 0))
        panic("routing_r dtt_rr: Skew combination not allowed");
    } else if (sk_xz != 0) {
      if ((sk_xy != 0) || (sk_yx != 0) || (sk_zx != 0) || (sk_zy != 0))
        panic("routing_r dtt_rr: Skew combination not allowed");
    } else if (sk_yx != 0) {
      if ((sk_yz != 0) || (sk_xy != 0) || (sk_xz != 0) || (sk_zy != 0))
        panic("routing_r dtt_rr: Skew combination not allowed");
    } else if (sk_yz != 0) {
      if ((sk_yx != 0) || (sk_xy != 0) || (sk_zx != 0) || (sk_zy != 0))
        panic("routing_r dtt_rr: Skew combination not allowed");
    } else if (sk_zx != 0) {
      if ((sk_zy != 0) || (sk_xy != 0) || (sk_xz != 0) || (sk_yz != 0))
        panic("routing_r dtt_rr: Skew combination not allowed");
    } else if (sk_zy != 0) {
      if ((sk_zx != 0) || (sk_xz != 0) || (sk_yx != 0) || (sk_yz != 0))
        panic("routing_r dtt_rr: Skew combination not allowed");
    } else { // If all skews are 0 then we are in a torus
      printf("WARNING: All skews of this twisted torus are 0.");
      printf("  		 Using torus topology instead!!!\n");
      topo = TORUS;
    }
  }
  // other topologies must be defined

  if (vc_management == BUBBLE_MANAGEMENT &&
      req_mode != BUBBLE_ADAPTIVE_RANDOM_REQ &&
      req_mode != BUBBLE_ADAPTIVE_SHORTEST_REQ &&
      req_mode != BUBBLE_ADAPTIVE_SMART_REQ && req_mode != BUBBLE_OBLIVIOUS_REQ
#if (BIMODAL_SUPPORT != 0)
      && req_mode != BIMODAL_REQ
#endif /* BIMODAL */
  )
    panic("Incorrect request mode specification for bubble VC management");
  else if (vc_management == DOUBLE_MANAGEMENT &&
           req_mode != DOUBLE_OBLIVIOUS_REQ &&
           req_mode != DOUBLE_ADAPTIVE_REQ && req_mode != HEXA_OBLIVIOUS_REQ &&
           req_mode != HEXA_ADAPTIVE_REQ)
    panic("Incorrect request mode specification for double VC management");
  else if (vc_management == DALLY_MANAGEMENT && req_mode != DALLY_TRC_REQ &&
           req_mode != DALLY_BASIC_REQ && req_mode != DALLY_IMPROVED_REQ &&
           req_mode != DALLY_ADAPTIVE_REQ)
    panic("Incorrect request mode specification for Dally VC management");

  if (pattern == HOTREGION && nprocs < 8)
    panic("Hotregion traffic pattern require more than 8 nodes");
  if (pattern == TRACE) {
    drop_packets =
        B_FALSE; // If some packet are dropped the simulation will never end.
    extract = 0; // Same as previous.
    samples = 1; // For final summary
    shotmode = B_FALSE;
    load = bg_load; // generation rate of the background traffic.
    if (trcfile == NULL)
      panic("Trace file undefined");
    printf("WARNING: Trace-driven mode\n");
    printf("         Disabling packet dropping & extracting!!!\n");
#if (BIMODAL_SUPPORT != 0)
    printf("         Setting off bimodal injection!!!\n");
    msglength = 1; // Bimodal injection not allowed while using traces.
    lm_percent = 0.0;
#endif /* BIMODAL */
    printf("         Setting samples to 1!!!\n");
    if (trigger_rate >= 0.0) {
      printf("         Deactivating causal synthetic traffic!!!\n");
      trigger_rate = 0.0;
    }
    // Let's check the placement parameters
    if (trace_nodes == 0) {
      trace_nodes = nprocs;
      trace_instances = 1;
    }
    if (trace_instances == 0)
      trace_instances = 1;

    if (placement == QUADRANT_PLACE) {
      if (topo < DIRECT) {
        trace_instances = (long)pow(2, ndim);
        printf("WARNING: quadrant placement with a %ld-D topology\n", ndim);
        printf("		 trace_instances set to %ld!!!\n",
               trace_instances);
      } else
        panic("Quadrant placement only for k-ary n-cube topologies");
    }
    if (placement == DIAGONAL_PLACE)
      if (ndim != 2 || trace_instances != 1 || topo > DIRECT)
        panic("Diagonal placement only for 2D cube topologies and 1 instance");
    if (trace_nodes * trace_instances > nprocs)
      panic("Too many nodes and/or instances for this trace. There are more "
            "tasks than nodes");
  }

  if (req_mode > TWO_OR_MORE_REQUIRED && nchan < 2)
    panic("Two or more virtual channels required");

  if (req_mode > THREE_OR_MORE_REQUIRED && nchan < 3)
    panic("Three or more virtual channels required");

  if (req_mode > SIX_REQUIRED && nchan != 6)
    panic("Six virtual channels are required");

  if (vc_management == DALLY_MANAGEMENT &&
      (bub_x != 0 || bub_y != 0 || bub_z != 0)) {
    printf("WARNING: Dally VC management selected\n");
    printf("         Setting bubbles to 0!!!\n");
    bub_x = bub_y = bub_z = 0;
  }

  if (shotmode) {
    if (shotsize == 0)
      shotsize = (nprocs - 1);
  }

  if (max_conv_time == 0)
    max_conv_time = (CLOCK_TYPE)1000000L; // Should have converged in less than
                                          // a million cycles.

#if (BIMODAL_SUPPORT != 0)
  lm_prob = lm_percent / (msglength - (lm_percent * (msglength - 1)));
  aload = (long)(load * RAND_MAX * (msglength * (1 - lm_prob) + lm_prob) /
                 (pkt_len * msglength));
  lm_load = aload * lm_prob;
#else
  // is the same as above when msglength=1 & lm_percent=0 (bimodal: off)
  aload = (long)((load / pkt_len) * RAND_MAX);
#endif /* BIMODAL */

  if (aload < 0) // Because an overflow
    aload = RAND_MAX;

  trigger = trigger_rate * RAND_MAX;
  trigger_dif = 1 + trigger_max - trigger_min;

  switch (topo) {
  case MESH:
    if (!update_period)
      update_period = 1;
    else
      update_period = update_period * (nodes_x + nodes_y + nodes_z - 3);
    break;
  case TORUS:
  case TWISTED:
    if (!update_period)
      update_period = 1;
    else
      update_period =
          update_period * ((nodes_x / 2) + (nodes_y / 2) + (nodes_z / 2));
    break;
  default:
    if (!update_period)
      update_period = 1;
    printf("WARNING: Using global congestion control with neither TORUS nor "
           "MESH topology\n");
    printf("         Setting update_period to %" PRINT_CLOCK " cycles!!!\n",
           update_period);
    break;
  }

  if (monitored > NUMNODES)
    monitored = -1;

  if (strcmp(file, "") == 0) {
#ifdef WIN32
    sprintf(file, "fsin.%ld.out", GetCurrentProcessId());
#else
    sprintf(file, "fsin.%ld.out", (long)getpid());
#endif
    printf("WARNING: Output files undefined. Using %s as default!!!\n", file);
  }
  if ((pheaders & 1024) && (monitored >= 0)) {
#if (EXECUTION_DRIVEN != 0)
    num_executions = 0;
    sprintf(mon, "%s.%ld.mon", file, num_executions);
#else
    sprintf(mon, "%s.mon", file);
#endif

    if ((fp = fopen(mon, "w")) == NULL)
      panic("Can not create monitorized output file");
  }

  if (timeout_upper_limit > 0 && timeout_lower_limit < 0)
    timeout_lower_limit = (timeout_upper_limit * 3) / 4;

  if (timeout_upper_limit < 1)
    bub_adap[0] = bub_adap[1];

  if (topo == ICUBE && routing != ADAPTIVE_ROUTING && nchan > 1) {
    printf("WARNING: multiple VCs not allowed with static routing in icubes\n");
    printf("         Setting nchan to 1!!!\n");
    nchan = 1;
  }

  if (topo != ICUBE &&
      (req_mode == ICUBE_REQ || vc_management == ICUBE_MANAGEMENT))
    panic("Only for indirect cubes");
  if ((topo != FATTREE && topo != THINTREE && topo != SLIMTREE && topo != RRG &&
       topo != EXA && topo != GDBG && topo != KAUTZ && topo != DRAGONFLY_RND &&
       topo != DRAGONFLY_ABSOLUTE && topo != DRAGONFLY_RELATIVE &&
       topo != DRAGONFLY_CIRCULANT && topo != DRAGONFLY_NAUTILUS &&
       topo != DRAGONFLY_HELIX && topo != MEGAFLY && topo != SLIMFLY &&
       topo != DRAGONFLY_OTHER) &&
      (req_mode == ARBITRARY_REQ || vc_management == TREE_MANAGEMENT))
    panic(
        "Tree request and/or management selected for a not supported topology");
  if (topo > CUBE)
    nodes_x = stDown; // This way the results will be printed in columns that
                      // are the number of nodes attached to each switch.

  fflush(stdout);
}

/**
 * The default configuration is set here.
 */
void set_default_conf(void) {
  r_seed = 17;

  pkt_len = 8;
  phit_len = 8;
  pattern = UNIFORM;
  load = 0.05;
  bg_load = 0.0;
  drop_packets = B_FALSE;
  extract = B_FALSE;

  topo = TORUS;

  nodes_x = 4;
  nodes_y = 4;
  nodes_z = 1;

  nways = 2;
  nchan = 2;
  ninj = 1;
  buffer_cap = 4;
  binj_cap = 4;
  tr_ql = buffer_cap * pkt_len + 1;
  inj_ql = binj_cap * pkt_len + 1;

  sk_xy = sk_xz = sk_yx = sk_yz = sk_zx = sk_zy = 0;

  vc_management = BUBBLE_MANAGEMENT;
  routing = DIMENSION_ORDER_ROUTING;
  req_mode = BUBBLE_ADAPTIVE_SMART_REQ;
  bub_x = bub_y = bub_z = 2;
  arb_mode = ROUNDROBIN_ARB;
  intransit_pr = 0.0;
  cons_mode = MULTIPLE_CONS;
  inj_mode = SHORTEST_INJ;
  parallel_injection = B_TRUE;
  plevel = 0;
  pinterval = (CLOCK_TYPE)1000L;
  pheaders = 2047;
  bheaders = 8191;
  monitored = 1;
  bub_adap[0] = 0;
  bub_adap[1] = 0;
  shotmode = B_FALSE;
  shotsize = 1;
  global_cc = 100.0;
  update_period = (CLOCK_TYPE)0L;

#if (BIMODAL_SUPPORT != 0)
  msglength = 8;
  lm_percent = 0.1;
#endif /* BIMODAL */

  warm_up_period = (CLOCK_TYPE)5000L;

  conv_period = (CLOCK_TYPE)1000L;
  threshold = 0.05;
  max_conv_time = (CLOCK_TYPE)25000L;
  trace_nodes = 0;
  trace_instances = 0;
  cpu_units = UNIT_NANOSECONDS;
  link_bw = 10000; // 10 Gbps
  samples = 10;
  batch_time = (CLOCK_TYPE)1000L;
  min_batch_size = 0;

  timeout_upper_limit = (CLOCK_TYPE)-1L;
  timeout_lower_limit = (CLOCK_TYPE)-1L;

  ndim = 2;
  radix = 4;
  nstages = 0;
  placement = CONSECUTIVE_PLACE;
  shift_plc = 0;
  stride = 1;

  faults = 0;

  trigger_rate = 0.0;
  trigger_max = 1;
  trigger_min = 1;
  cam_policy = CAM_SP;
  cam_policy_params[0] = 1;
  cam_policy_params[1] = -1;
  cam_policy_params[2] = -1;
  vc_inj = VC_INJ_ZERO;

  nnics = 1;

#if (EXECUTION_DRIVEN != 0)
  fsin_cycle_relation = 10;
  simics_cycle_relation = 100;
  serv_addr = 8082;
  num_periodos_espera = 0;
#endif
}
