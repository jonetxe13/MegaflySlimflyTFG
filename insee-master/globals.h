/**
* @file
* @brief	Declaration of all global variables & functions.

FSIN Functional Simulator of Interconnection Networks
Copyright (2003-2011) J. Miguel-Alonso, J. Navaridas

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

#ifndef _globals
#define _globals

#include <stdlib.h>

#include "misc.h"
#include "constants.h"
#include "literal.h"
#include "phit.h"
#include "packet.h"
#include "queue.h"
#include "event.h"
#include "router.h"
#include "pkt_mem.h"
#include "batch.h"
#include "graph.h"
#include "spanning_tree.h"

#include <math.h>
#include <time.h>
#include <limits.h>

#define packet_size_in_phits pkt_len

extern long radix;
extern long ndim;
extern long nstages;
extern long stDown;
extern long stUp;
extern long NUMNODES;
extern long nprocs;
extern long nnics;
extern long n_ports;

extern long r_seed;
extern long nodes_x, nodes_y, nodes_z;
extern long binj_cap;
extern long ninj;
extern router  * network;
extern long **destinations;
extern long **sources;
extern long * con_dst;
extern long * inj_dst;
extern long max_dst;
extern long *source_ports;
extern long *dest_ports;
extern CLOCK_TYPE *port_utilization;
extern double sent_count;
extern double injected_count;
extern double rcvd_count;
extern double last_rcvd_count;
extern double dropped_count;
extern double transit_dropped_count;
extern double last_tran_drop_count;

extern long pnodes_x;
extern long pnodes_y;
extern long pnodes_z;

extern long nswitches;

extern int switches, servers;
extern long param_q; ///< q: Prime number that defines slimfly graph
extern int param_k; ///< k: radix in slimfly
extern long param_p; ///< p: Number of servers connected to each switch
extern long param_a; ///< a: Number of switches in each group
extern long param_h; ///< h: Number of uplinks
extern int param_delta_opt[3];
extern int param_delta_final;
extern int param_tam_gal;
extern int *param_x, *param_x2;

extern long grps; ///< Total number of groups
extern long intra_ports; ///<  Total number of ports in one group connecting to other routers in the group

extern long stride; ///< stride to be used with the shift and groupshift patterns
extern long group_size; ///< group size to be used with the groupshift pattern

extern char topology_filename[128];

extern long faults;

#if (BIMODAL_SUPPORT != 0)
 extern double msg_sent_count[3];
 extern double msg_injected_count[3];
 extern double msg_rcvd_count[3];

 extern double msg_acum_delay[3], msg_acum_inj_delay[3];
 extern long msg_max_delay[3], msg_max_inj_delay[3];
 extern double msg_acum_sq_delay[3], msg_acum_sq_inj_delay[3];

 extern long msglength;
 extern double lm_prob, lm_percent;
#endif /* BIMODAL */

extern CLOCK_TYPE sim_clock;
extern CLOCK_TYPE last_reset_time;
extern long bub_adap[2], bub_x, bub_y, bub_z;
extern topo_t topo;
extern long plevel;
extern long pheaders;
extern long bheaders;
extern CLOCK_TYPE pinterval;
extern bool_t extract;;
extern long monitored;
extern double intransit_pr;

extern double inj_phit_count;
extern double sent_phit_count;
extern double rcvd_phit_count;
extern double dropped_phit_count;

extern long (*neighbor)(long ad, dim wd, way ww);
extern routing_r (*calc_rr)(long source, long destination);
extern void (*request_port) (long i, port_type s_p);
extern void (*arbitrate_cons)(long i);
extern port_type (*arbitrate_select)(long i, port_type d_p, port_type first, port_type last);
extern void (*consume)(long i);
extern bool_t (*check_rr)(packet_t * pkt, dim *d_d, way *d_w);
extern port_type (* select_input_port) (long i, long dest);
extern void (*data_movement)(bool_t inject);
extern void (*arbitrate)(long i, port_type d_p);
extern void (*init_cams)();
extern void (*finish_cams)();
extern void (*fill_cam)(long switch_id, graph_t *graph);
extern void (*set_mpaths)(path_t **path, long n_paths);
extern void (*set_mpaths_reverse)(path_t **path, long n_paths);
extern long (*get_next_hop) (packet_t * pkt);
extern long (*get_next_router_hop) (packet_t * pkt);
extern long (*get_cam_port) (packet_t * pkt);

extern double load, trigger_rate ;
extern long aload, lm_load, trigger;
extern long link_bw;
extern long trigger_max, trigger_min, trigger_dif;
extern double global_q_u, global_q_u_current;
extern vc_management_t vc_management;
extern routing_t routing;
extern cam_policy_t cam_policy;
extern vc_inj_t vc_inj;
extern cam_ports_t cam_ports;
extern long cam_policy_params[3];
extern traffic_pattern_t pattern;
extern cpu_units_t cpu_units;
extern cons_mode_t cons_mode;
extern arb_mode_t arb_mode;
extern req_mode_t req_mode;
extern inj_mode_t inj_mode;

extern placement_t placement;
extern long shift_plc;
extern long trace_nodes;
extern long trace_instances;
extern char placefile[128];

#if (SKIP_CPU_BURSTS==1)
extern CLOCK_TYPE skipped_cycles, skipped_periods;
#endif

extern bool_t drop_packets;
extern bool_t parallel_injection;
extern bool_t shotmode;
extern long shotsize;
extern double global_cc;
extern long congestion_limit;
extern CLOCK_TYPE timeout_upper_limit;
extern CLOCK_TYPE timeout_lower_limit;

extern CLOCK_TYPE update_period;
extern long total_shot_size;

extern batch_t *batch;
extern long samples;
extern CLOCK_TYPE batch_time;
extern double threshold;
extern CLOCK_TYPE warm_up_period, warmed_up;
extern CLOCK_TYPE conv_period;
extern CLOCK_TYPE max_conv_time;
extern long min_batch_size;

extern double acum_delay, acum_inj_delay;
extern long max_delay, max_inj_delay;
extern double acum_sq_delay, acum_sq_inj_delay;
extern double acum_hops;

extern long links_per_direction;

extern long pkt_len, phit_len, buffer_cap, tr_ql, inj_ql;

extern dim * port_coord_dim;
extern way * port_coord_way;
extern channel * port_coord_channel;

extern packet_t * pkt_space;
extern long pkt_max;

extern char trcfile[256];

extern bool_t go_on;
extern bool_t interrupted;
extern bool_t aborted;

extern long reseted;
extern double threshold;

extern FILE *fp;
extern char file[256];

extern port_type last_port_arb_con;

extern void (* run_network)(void);

void run_network_shotmode(void);
void run_network_batch(void);

void report_receiving_tasks(void);

/* In data_generation.c */
void init_injection(void);
void injection_finish(void);
void data_generation(long i);
void data_injection(long i);
void datagen_oneshot(bool_t reset);

void generate_pkt(long i);
port_type select_input_port_shortest(long i, long dest);
port_type select_input_port_dor_only(long i, long dest);
port_type select_input_port_dor_shortest(long i, long dest);
port_type select_input_port_shortest_profitable(long i, long dest);
port_type select_input_port_lpath(long i, long dest);

/* In arbitrate.c */
void reserve(long i, port_type d_p, port_type s_p);
void arbitrate_init(void);
void arbitrate_finish(void);
void arbitrate_cons_single(long i);
void arbitrate_cons_multiple(long i);
void arbitrate_direct(long i, port_type d_p);
void arbitrate_icube(long i, port_type d_p);
void arbitrate_arbitrary(long i, port_type d_p);
port_type arbitrate_select_fifo(long i, port_type d_p, port_type first, port_type last);
port_type arbitrate_select_longest(long i, port_type d_p, port_type first, port_type last);
port_type arbitrate_select_highest(long i, port_type d_p, port_type first, port_type last);
port_type arbitrate_select_round_robin(long i, port_type d_p, port_type first, port_type last);
port_type arbitrate_select_random(long i, port_type d_p, port_type first, port_type last);
port_type arbitrate_select_age(long i, port_type d_p, port_type first, port_type last);

/* In request_ports.c */
void request_ports_init(void);
void request_ports_finish(void);
void request_port_bubble_oblivious(long i, port_type s_p);
void request_port_bubble_adaptive_random(long i, port_type s_p);
void request_port_bubble_adaptive_shortest(long i, port_type s_p);
void request_port_bubble_adaptive_smart(long i, port_type s_p);
void request_port_double_oblivious(long i, port_type s_p);
void request_port_double_adaptive(long i, port_type s_p);
void request_port_hexa_oblivious(long i, port_type s_p);
void request_port_hexa_adaptive(long i, port_type s_p);
void request_port_dally_trc(long i, port_type s_p);
void request_port_dally_basic(long i, port_type s_p);
void request_port_dally_improved(long i, port_type s_p);
void request_port_dally_adaptive(long i, port_type s_p);
void request_port_bimodal_random(long i, port_type s_p);

void request_port_icube(long i, port_type s_p);
void request_port_icube_IB(long i, port_type s_p);
void request_port_arbitrary(long i, port_type s_p);

bool_t check_rr_dim_o_r(packet_t * pkt, dim *d_d, way *d_w);
bool_t check_rr_dir_o_r(packet_t * pkt, dim *d_d, way *d_w);
bool_t check_rr_fattree(packet_t * pkt, dim *d_d, way *d_w);
bool_t check_rr_fattree_adaptive(packet_t * pkt, dim *d, way *w);
bool_t check_rr_thintree_adaptive(packet_t * pkt, dim *d, way *w);
bool_t check_rr_slimtree_adaptive(packet_t * pkt, dim *d, way *w);
bool_t check_rr_icube_adaptive(packet_t * pkt, dim *d, way *w);
bool_t check_rr_icube_static_IB(packet_t * pkt, dim *d, way *w);
bool_t check_rr_fattree_arithmetic(packet_t * pkt, dim *d, way *w);
bool_t check_rr_thintree_arithmetic(packet_t * pkt, dim *d, way *w);
bool_t check_rr_slimtree_arithmetic(packet_t * pkt, dim *d, way *w);
bool_t check_rr_icube_static(packet_t * pkt, dim *d, way *w);
bool_t check_rr_graph_sp(packet_t * pkt, dim *d_d, way *d_w);
bool_t check_rr_arbitrary_node(packet_t * pkt, dim *d, way *w);
bool_t check_rr_arbitrary_port(packet_t * pkt, dim *d, way *w);
bool_t check_rr_arbitrary_node_port(packet_t * pkt, dim *d, way *w);
bool_t check_rr_arbitrary_dummy(packet_t * pkt, dim *d, way *w);
bool_t check_rr_arbitrary_voq(packet_t * pkt, dim *d, way *w);
bool_t check_rr_dragonfly_arithmetic(packet_t * pkt, dim *d, way *w);
bool_t check_rr_dragonfly_dally(packet_t * pkt, dim *d, way *w);
bool_t check_rr_dragonfly_dally_adap(packet_t * pkt, dim *d, way *w);
bool_t check_rr_megafly_arithmetic(packet_t * pkt, dim *d, way *w);
bool_t check_rr_megafly_dally(packet_t * pkt, dim *d, way *w);
bool_t check_rr_megafly_dally_adap(packet_t * pkt, dim *d, way *w);
bool_t check_rr_slimfly_arithmetic(packet_t * pkt, dim *d, way *w);
bool_t check_rr_slimfly_dally(packet_t * pkt, dim *d, way *w);
bool_t check_rr_slimfly_dally_adap(packet_t * pkt, dim *d, way *w);
bool_t check_rr_spanning_tree(packet_t * pkt, dim *d, way *w);
bool_t check_rr_arbitrary_inc_hop(packet_t * pkt, dim *d, way *w);

long get_next_hop_rr(packet_t*pkt);
long get_next_hop_cam(packet_t*pkt);
long get_next_router_hop_rr(packet_t*pkt);
long get_next_router_hop_cam(packet_t*pkt);
long get_cam_port_rr(packet_t * pkt);
long get_cam_port_adaptive_node(packet_t * pkt);
long get_cam_port_adaptive_port(packet_t * pkt);
long get_cam_port_adaptive_node_port(packet_t * pkt);

/* In perform_mov.c */
void phit_away(long, port_type, phit);
void consume_single(long i);
void consume_multiple(long i);
void advance(long n, long p);
void data_movement_direct(bool_t inject);
void data_movement_indirect(bool_t inject);

/* In init_functions.c */
void init_functions (void);
void finish_functions (void);

/* In stats.c */
void stats(long i);
void reset_stats(void);

/* In router.c */
extern long nchan;
extern long used_chan;
extern long diameter_t;
extern long diameter_r;
extern long nways;
extern port_type p_con;
extern port_type p_drop;
extern port_type p_inj_first, p_inj_last;

void init_ports(long i);
void router_init(void);
void router_finish(void);
void init_network(void);
void finish_network(void);
void coords (long ad, long *cx, long *cy, long *cz);
void coords_icube (long ad, long *cx, long *cy, long *cz);

long torus_neighbor(long ad, dim wd, way ww);
long midimew_neighbor(long ad, dim wd, way ww);
long dtt_neighbor(long ad, dim wd, way ww);
long circulant_neighbor(long ad, dim wd, way ww);
long circ_pk_neighbor(long ad, dim wd, way ww);
long spinnaker_neighbor(long ad, dim wd, way ww);

routing_r torus_rr (long source, long destination);
routing_r torus_rr_unidir (long source, long destination);
routing_r mesh_rr (long source, long destination);
routing_r midimew_rr (long source, long destination);
routing_r circulant_rr (long source, long destination);
routing_r circ_pk_rr (long source, long destination);
routing_r dtt_rr (long source, long destination);
routing_r dtt_rr_unidir (long source, long destination);
routing_r icube_rr (long source, long destination);
routing_r icube_4mesh_rr (long source, long destination);
routing_r icube_1mesh_rr (long source, long destination);
routing_r fattree_rr_arithmetic (long source, long destination);
routing_r fattree_rr (long source, long destination);
routing_r thintree_rr_arithmetic (long source, long destination);
routing_r thintree_rr (long source, long destination);
routing_r thintree_rr_dst (long source, long destination);
routing_r thintree_rr_src (long source, long destination);
routing_r thintree_rr_rnd (long source, long destination);
routing_r slimtree_rr_arithmetic (long source, long destination);
routing_r slimtree_rr (long source, long destination);
routing_r spinnaker_rr(long source, long destination);
routing_r graph_rr_static(long source, long destination);
routing_r graph_rnd_static(long source, long destination);
routing_r graph_rr_adaptive(long source, long destination);
routing_r dragonfly_rr (long source, long destination);
routing_r megafly_rr (long source, long destination);
routing_r slimfly_rr (long source, long destination);
routing_r spanning_tree_rr(long source, long destination);

extern struct spanning_tree_routing_table_t *s_t_routing_table;

long route_dragonfly(long current, long destination, long proxy);
long route_megafly(long current, long destination, long *proxy);
long route_slimfly(long current, long destination, long proxy);

void create_fattree();
void create_slimtree();
void create_thintree();
void create_icube();
void create_graph();
void create_dragonfly();
void finish_dragonfly();
void create_megafly();
void finish_megafly();
void create_slimfly();
void finish_slimfly();

/* In get_conf.c */
extern literal_t vc_l[];
extern literal_t routing_l[];
extern literal_t rmode_l[];
extern literal_t atype_l[];
extern literal_t ctype_l[];
extern literal_t pattern_l[];
extern literal_t cpu_units_l[];
extern literal_t topology_l[];
extern literal_t injmode_l[];
extern literal_t placement_l[];

void get_conf(long, char **);

/* In print_results.c */
void print_headers(void);
void print_partials(void);
void print_results(time_t, time_t);
void results_partial(void);

/* In batch.c */
void save_batch_results();
void print_batch_results(batch_t *b);
void print_batch_results_vast(batch_t *b);

/* In circulant.c */
extern long step;	// 2nd dimension of a circulant graph
extern long twist;
extern long rows;

/* In circ_pk.c */
long gcd(long i, long n);
long inverse(long i, long n);
extern long a_circ, k_circ;
extern long s1, s2;
extern long k_inv;

/* In dragonfly.c */
extern long escape_vcs;

/* In dtt.c */
extern long sk_xy, sk_xz, sk_yx, sk_yz, sk_zx, sk_zy; // Skews for twisted torus

/* In queue.c */
void init_queue (queue *q);

/* In pkt_mem.c */
void pkt_init();
void pkt_finish();
void free_pkt(unsigned long n);
unsigned long get_pkt();

#if (TRACE_SUPPORT != 0)
 /* In trace.c */
 void read_trace();
 void trace_finish();
 void run_network_trc();

/* In event.c */
 void init_event (event_q *q);
 void ins_event (event_q *q, event i);
 void do_event (event_q *q, event *i);
 void do_event_n_times (event_q *q, event *i, CLOCK_TYPE increment);
 event head_event (event_q *q);
 void rem_head_event (event_q *q);
 bool_t event_empty (event_q *q);
#endif /* TRACE common */

#if (TRACE_SUPPORT > 1)
 void init_occur (event_l **l);
 void ins_occur (event_l **l, event i);
 bool_t occurred (event_l **l, event i);
#endif /* TRACE multilist */

#if (TRACE_SUPPORT == 1)
 void init_occur (event_l *l);
 void ins_occur (event_l *l, event i);
 bool_t occurred (event_l *l, event i);
#endif /* TRACE single list */

#if (EXECUTION_DRIVEN != 0)
  extern long fsin_cycle_relation;
  extern long simics_cycle_relation;
  extern long serv_addr;
  extern long num_periodos_espera;
  extern long num_executions;
  void init_exd(long, long, long, long, long);
  void run_network_exd(void);
#endif

#endif /* _globals */

