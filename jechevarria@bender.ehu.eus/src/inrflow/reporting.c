#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "reporting.h"
#include "misc.h"
#include "globals.h"

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

long long link_hops;///< overall number of links traversed (slightly different from server_hops).
long long server_hops;///< overall number of server-to-server hops (slightly different from link_hops)
long bottleneck_flow;///<aggregated bisection throughput
long upper_flows;

long *server_hop_histogram;
long *path_length_histogram;
long *flows_histogram;

long **trace_matrix;///< Trace matrix records connected flows. Memory intensive,
///usually off.

static time_t start_time;	///< Simulation start time / date.
static time_t end_time;		///< Simulation finish time / date.

char time_started[26];
struct tm* tm_info;

FILE *stats_file, *hist_file, *tex_file, *trace_file;
char filename_params[300];
char stats_filename[300], hist_filename[300], prefix_filename[300], tex_filename[300], trace_filename[300];
char sys_command[1000];

char * inrflow_version="v0.4.1";
long long connected;///< number of connected pairs (server-to-server)

/**
 * construct key_value node and add it to stats_head linked list
 *
 */
void add_key_value(struct key_value ** stats_head, char* key,const char* value_format, ...)
{
    char value[50];
    va_list arg;
    va_start(arg,value_format);
    vsprintf(value,value_format,arg);
    va_end(arg);

    if(stats_head==NULL)
    {
        *stats_head=malloc(sizeof(struct key_value));
        strcpy((*stats_head)->key,key);
        strcpy((*stats_head)->value,value);
    }
    else
    {
        struct key_value* new_node;
        new_node=malloc(sizeof(struct key_value));
        strcpy(new_node->key,key);
        strcpy(new_node->value,value);
        new_node->next=*stats_head;
        *stats_head=new_node;
    }
}


/**
 * @brief provide a key and value and output as pair.
 *
 * Key is a pointer to a string and value_format should be a string containing a
 * single formatted values.  Provide a value in the third argument.  Although
 * the implementation provides more freedom, any spaces in the value will likely
 * make the output unreadable by various programs like pgfplots.
 *
 * @param key String describing value.
 * @param value_format  format string that will not produce spaces
 * @param ...  one or more values (but preferably one)
 * @return key_value
 */
key_value make_key_value_node(char *key,const char* value_format, ...)
{
    key_value kv;
    char value[50];
    va_list arg;
    va_start(arg,value_format);
    vsprintf(value,value_format,arg);
    va_end(arg);

    strcpy(kv.key,key);
    strcpy(kv.value,value);
    //printf("REP MAK: %s %s\n",kv.key,kv.value);
    return kv;
}

/**
 * instert a "ready-made" struct key_value
 *
 */
void insert_key_value(key_value ** stats_head, key_value kv)
{
    if(stats_head==NULL)
    {
        *stats_head=malloc(sizeof(key_value));
        memcpy(*stats_head,&kv,sizeof(key_value));
    }
    else
    {
        struct key_value *new_node;
        new_node=malloc(sizeof(struct key_value));
        memcpy(new_node,&kv,sizeof(key_value));
        new_node->next=*stats_head;
        *stats_head=new_node;
    }
    //printf("REP INS: %s %s\n",(*stats_head)->key,(*stats_head)->value);
}


void reverse_list(struct key_value ** stats_head)
{
    struct key_value* curr_node=NULL, *prev_node=NULL;
    curr_node=*stats_head;
    while(curr_node!=NULL)
    {
        curr_node=curr_node->next;
        (*stats_head)->next=prev_node;
        prev_node=*stats_head;
        *stats_head=curr_node;
    }
    *stats_head=prev_node;
}

void print_keys(struct key_value* stats_head, FILE * output)
{
    struct key_value* curr_node;
    curr_node=stats_head;
    while(curr_node!=NULL)
    {
        fprintf(output,"%s ",curr_node->key);
        curr_node=curr_node->next;
    }
    fprintf(output,"\n");
}

void print_values(struct key_value* stats_head, FILE * output)
{
    struct key_value* curr_node;
    curr_node=stats_head;
    while(curr_node!=NULL)
    {
        fprintf(output,"%s ",curr_node->value);
        curr_node=curr_node->next;
    }
    fprintf(output,"\n");
}

void print_key_value_pairs(struct key_value* stats_head, FILE * output)
{
    struct key_value* curr_node;
    curr_node=stats_head;
    while(curr_node!=NULL)
    {
        fprintf(output,"%-35s ",curr_node->key);
        fprintf(output,"%-20s\n",curr_node->value);
        curr_node=curr_node->next;
    }
}

long print_hist_array(long *H, long n,char prefix)
{
    long i;
    for (i=0; i<n; i++)
    {
        if(H[i]>0)
            printf("%c %4ld %9ld\n",prefix,i,H[i]);
    }
    return 1;
}

long print_hist_next(FILE * output, long *H, long upper, long index, double normalise)
{
    while(index<upper && H[index]==0)index++;
    if(index<upper)
        fprintf(output,"%9ld %9ld %1.5f ",index,H[index],H[index]/normalise);
    else
        fprintf(output,"%9s %9s %7s ","nan", "nan", "nan");
    return ++index;
}

long print_init_topo()
{

    return 1;
}
/**
 * Various utilities for collecting and computing stats
 *
 */

long init_hist_array(long ** H, long n)
{
    long i;
    *H = malloc(n*(sizeof(long)));
    for (i=0; i<n; i++)
    {
        (*H)[i] = 0;
    }
    return 1;

}

void finish_hist_array(long **H)
{
    free(*H);
}

long update_hist_array(long hops, long ** H, long n)
{
    if(hops>=n)
    {
        printf("ERROR:  Histogram update exceeds maximum index:  value %ld, maximum %ld.\n",hops,n);
        exit(-1);
    }
    (*H)[hops]++;
    return 1;
}


long obtain_flows()
{
    long i,j;
    for (i=0; i<servers+switches; i++)
    {
        for (j=0; j<network[i].nports; j++)
        {
            //ensure that faulty and open links are not counted,
            //since, we do not want to skew things by these
            //0-flows (or worse, garbage-flows).
            if(!network[i].port[j].faulty &&
                    !(network[i].port[j].neighbour.node==-1))
                flows_histogram[network[i].port[j].flows]++;
        }
    }
    return 1;
}

double mean_hist_array(long * H, long n)
{
    long i,m,total;
    m=0;
    total=0;
    for (i=0; i<n; i++)
    {
        m+=H[i]*i;
        total+=H[i];
    }
    return ((double)m)/((double)total);
}

long max_hist_array(long *H,long n)
{
    long i;
    for (i=n-1; i>=0; i--)
    {
        if(H[i]>0)
            return i;
    }
    return 0;
}

long min_hist_array(long *H,long n)
{
    long i;
    for (i=0; i<n; i++)
    {
        if(H[i]>0)
            return i;
    }
    return 0;
}

long long size_hist_array(long *H,long n)
{
    long i;
    long long total;
    total=0;
    for (i=0; i<n; i++)
    {
        total+=(long long)H[i];
    }
    return total;
}

//This is the mean of the list: 0{repeated H[0] times}, 1{repeated H[1] times}, etc
//H[0]=2
//H[3]=2
//median is 1.5, but the old function would have output 0

//H[0]=1
//H[1]=1
//H[2]=1
//median is 1 but old function would have output 0
// AE: I think I fixed the bias here
double median_hist_array(long * H, long n)
{
    long i,cum,j;
    long long N2=size_hist_array(H,n)/2;//floor of half the number of things recorded in histogram
    cum=0;
    for (i=0; i<n; i++)
    {
        cum+=H[i];
        if(cum==N2)   //floor of half the number of things in histogram
        {
            //are in H[0] to H[i].
            //find the next non-zero index j
            j=i+1;
            while(H[j]==0 && j<(n-1)) j++;
            if(N2%2==0) //even number of things, return average of these two values.
                return (double)(i+j)/2;
            else //odd number of things, return j
                return (double)j;
        }
        else if(cum>N2)
            return (double)i;
    }
    return 0;
}

//variance
double var_hist_array(long *H, long n)
{
    long i,total;
    long long sum_of_squares, sum_squared;
    total = size_hist_array(H,n);
    sum_of_squares=0;
    sum_squared=0;
    for (i=0; i<n; i++)
    {
        sum_of_squares+=H[i]*i*i;
        sum_squared+=H[i]*i;
    }
    sum_squared *=sum_squared;
    return (double)((long double)sum_of_squares-(long double)sum_squared/total)/(total-1);
}

double std_dev_hist_array(long *H, long n)
{
    return (double)sqrt(var_hist_array(H,n));
}


void init_stats()
{
    time(&start_time);
    tm_info = localtime(&start_time);
    strftime(time_started, 26, "%Y.%m.%d.%H.%M",  tm_info);
    init_trace_matrix();
    init_hist_array(&path_length_histogram,UPPER_PATH_LENGTH);
    init_hist_array(&server_hop_histogram,UPPER_SERVER_HOPS);
    connected=0;
    link_hops=0;
    server_hops=0;
    bottleneck_flow=0;
}

void init_trace_matrix()
{
    int i,j;

    if(TRACE_MATRIX)
    {
        trace_matrix = malloc(servers * sizeof *trace_matrix);
        for(i = 0; i < servers; i++)
        {
            trace_matrix[i] = malloc(servers * sizeof **trace_matrix);
            for(j = 0; j < servers; j++)
            {
                trace_matrix[i][j] = 0;
            }
        }
    }
}

void destroy_trace_matrix()
{
    int i;

    if(TRACE_MATRIX)
    {
        for(i = 0; i < servers; i++)
        {
            free( trace_matrix[i] );
        }
        free(trace_matrix);
    }
}

void finish_stats(struct key_value ** stats_head)
{
    struct key_value* curr_node=NULL, *aux_node=NULL;

    curr_node=*stats_head;
    while(curr_node!=NULL)
    {
        aux_node = curr_node;
        curr_node = curr_node->next;
        free(aux_node);
    }
}


/**
 * Updates path length statistics after a flow.
 * @param sh path length in terms of server hops
 * @param lh path length in terms of link hops
 */
void update_statistics(long sh, long lh)
{
    update_hist_array(sh, &server_hop_histogram, UPPER_SERVER_HOPS);
    update_hist_array(lh, &path_length_histogram, UPPER_PATH_LENGTH);
    link_hops+=lh;
    server_hops+=sh;
}

void update_trace_matrix(long src, long dst)
{
    if(TRACE_MATRIX)
    {
        trace_matrix[src][dst]++;
    }
}

#ifdef DEBUG
/**
 * Checks consistency between various statistics.
 *
 */
void stats_consistency_check()
{
    if(n_failures==0 &&
            ports!=size_hist_array(flows_histogram,upper_flows))
    {
        printf("Flow histogram size and number of ports are inconsistent (%lld vs %ld).\n",size_hist_array(flows_histogram,upper_flows),ports);
        exit(-1);
    }

    if(n_failures==0 && pattern == ALL2ALL &&
            connected != ((long long)servers)*((long long)servers))
    {
        printf("FAIL CONSISTENCY CHECK: n_failures==0"
               " && pattern == ALL2ALL && connected != "
               "((long long)servers)*((long long)servers.\n instead got %lld and %lld.\n",
               connected,((long long)servers)*((long long)servers));
        exit(-1);
    }
    // This test gives false failures, possibly because of rounding error / casting
    /*	if(n_failures==0 &&
            ((long)(1000*mean_hist_array(flows_histogram, upper_flows))) !=
            ((long)((1000*connected *
                     mean_hist_array(path_length_histogram, UPPER_PATH_LENGTH) /
                     ports)))) {
    	printf("Mean number of flows per link computed with "
    	       "n.connected x mean.path.length / ports "
    	       "inconsistent with the flow histogram, to three decimal places.\n"
    	       "Reported:\n%f\n%f.\n",
    	       connected *
    	       mean_hist_array(path_length_histogram, UPPER_PATH_LENGTH) /
    	       ports,
    	       mean_hist_array(flows_histogram, upper_flows)
    				 );
    print_hist_array(path_length_histogram,UPPER_PATH_LENGTH,'p');
    exit(-1);
     }
    */
}
#endif

void compute_bottleneck_flow()
{
    int i,j;

    for(i=0; i<servers+switches; i++)
        for(j=0; j<network[i].nports; j++)
            if(network[i].port[j].flows>bottleneck_flow)
                bottleneck_flow=network[i].port[j].flows;
}

//writes file from trace matrix if trace was collected. Must be called after
//prefix_filename
void write_trace_file()
{
    int i,j;

    if(TRACE_MATRIX)
    {
        snprintf(trace_filename,300,"%s/%s_%s_trace.dat", output_dir, prefix_filename,traffic_name);
        trace_file = fopen(trace_filename, "w");
        if(!trace_file)
        {
            perror("Unable to open trace file");
            exit(-1);
        }
        fprintf(trace_file,
                "#Trace matrix records number of traffic flows from src to dst.\n"
                "#It does not reflect number of flows on any particular link.\n"
                "src dst traffic.flows\n");
        for(i = 0; i < servers; i++)
        {
            for(j = 0; j < servers; j++)
            {
                fprintf(trace_file,"%d %d %ld\n",i,j,trace_matrix[i][j]);
            }
        }
        printf("Wrote trace to file: %s\n",trace_filename);
        fclose(trace_file);
    }
}
void capture_statistics()
{
    long i;
    double runtime;
	unsigned long cn_size = 64;
	char computer_name[64];
    compute_bottleneck_flow();

//#define PRINT_CONGESTED_FLOWS
#ifdef PRINT_CONGESTED_FLOWS
    long j;
    long limit = 0;
    limit=9*bottleneck_flow/10;
    for(i=0; i<servers+switches; i++)
        for(j=0; j<network[i].nports; j++)
            if(network[i].port[j].flows>limit && is_server(i))
                print_coordinates(i,j);

#endif // PRINT_CONGESTED_FLOWS

    upper_flows = bottleneck_flow+1;
    init_hist_array(&flows_histogram,upper_flows);
    obtain_flows();

    //end of experiment.  only printing to do
    time(&end_time);
    runtime=difftime(end_time,start_time);

    sprintf(prefix_filename,"%s_%s_%s_%s_fr%.2f_seed%ld.%s",
            get_network_token(),
            get_filename_params(),
            get_routing_token(),
            placement_name,
            failure_rate,
            r_seed,
            time_started);
    write_trace_file();
    snprintf(stats_filename,300,"%s/%s.dat",output_dir, prefix_filename);
    stats_file = fopen(stats_filename,"w");
    if(stats_file == NULL)
    {
        perror("Unable to open stats file");
        exit(-1);
    }
    printf("Wrote stats to file: %s\n",stats_filename);

    long max_flow=max_hist_array(flows_histogram,upper_flows);
    long long server_pairs, non_zero_path_server_pairs;
    server_pairs=((long long)servers)*((long long)servers);
    non_zero_path_server_pairs = ((long long)servers)*((long long)(servers-1));

    struct key_value *stats_head=NULL;
    add_key_value(&stats_head,
                  "inrflow.version","%s",
                  inrflow_version );  //version of the inrflow code
    add_key_value(&stats_head,
                  "network", "" );  //spacer
    add_key_value(&stats_head,
                  "topology","%s",
                  get_network_token() );  //single-token name

    for(i=0; i<topo_nparam; i++)
    {
        add_key_value(
            &stats_head, get_topo_param_tokens(i),"%ld",
            topo_params[i]);
    }
    add_key_value(&stats_head,
                  "topo.version","%s",
                  get_topo_version() );  //version of the topology code
	add_key_value(&stats_head,
                  "n.servers","%ld",
                  servers );  // number of servers
    add_key_value(&stats_head,
                  "n.switches","%ld",
                  switches );  // number of switches
    add_key_value(&stats_head,
                  "switch.radix","%ld",
                  radix );  // radix of switches
    add_key_value(&stats_head,
                  "n.links","%lld",
                  size_hist_array(flows_histogram,
                                  upper_flows) );
    add_key_value(&stats_head,
                  "n.cables","%ld",
                  ports/2); // number of bidirectional links.
    add_key_value(&stats_head,
                  "p.cable.failures","%f",
                  failure_rate );  // proportion of failures
    add_key_value(&stats_head,
                  "n.cable.failures","%ld",
                  n_failures );  // proportion of failures
    add_key_value(&stats_head,
                  "n.server.pairs.NN","%lld",
                  server_pairs);  //servers*servers
    add_key_value(&stats_head,
                  "n.server.pairs.NN-1","%lld",
                  non_zero_path_server_pairs );   //servers*(servers-1)

    add_key_value(&stats_head,
                  "routing.algorithm","%s",
                  get_routing_token()  );  //single-token name
    if (routing_nparam>0 && get_routing_param_tokens!= NULL){
        for(i=0; i< routing_nparam; i++)
        {
            add_key_value(
                &stats_head,get_routing_param_tokens(i),"%ld",
                routing_params[i]);
        }
    }
    add_key_value(&stats_head,
                  "traffic","");  //spacer
    add_key_value(&stats_head,
                  "pattern","%s",
                  traffic_name );  //traffic pattern name
    for (i = 0; i < traffic_nparam; i++)
    {
        add_key_value(
            &stats_head,traffic_param_tokens[i],"%ld",
            traffic_params[i]);
    }
    add_key_value(&stats_head,
                  "n.flows","%lld",
                  traffic_npairs);  //servers*servers
    add_key_value(&stats_head,
                  "n.flows.delivered","%lld",
                  connected );  //global variable: connected
    add_key_value(&stats_head,
                  "p.flows.connected","%f",
                  (double)(100.0L*connected/traffic_npairs) );

    add_key_value(&stats_head,
                  "placement.strategy","%s",
                  placement_name );  //single-token name
    add_key_value(&stats_head,
                  "miscelanea","");  //spacer
    add_key_value(&stats_head,
                  "r.seed","%ld",
                  r_seed );  //random seed

    add_key_value(&stats_head,
                  "started.at.y.m.d.h.m","%s",
                  time_started);
	gethostname(computer_name, cn_size);
	add_key_value(&stats_head,
                  "on machine","%s",
                  computer_name);
	add_key_value(&stats_head,
                  "runtime","%f",
                  runtime);

    add_key_value(&stats_head,
                  "path.info","");  //spacer
    add_key_value(&stats_head,
                  "min.link.path.length","%ld",
                  min_hist_array(path_length_histogram,
                                 UPPER_PATH_LENGTH) );
    add_key_value(&stats_head,
                  "max.link.path.length","%ld",
                  max_hist_array(path_length_histogram,
                                 UPPER_PATH_LENGTH) );
    add_key_value(&stats_head,
                  "mean.link.path.length","%f",
                  mean_hist_array(path_length_histogram,
                                  UPPER_PATH_LENGTH) );
    add_key_value(&stats_head,
                  "median.link.path.length","%f",
                  median_hist_array(path_length_histogram,
                                    UPPER_PATH_LENGTH) );
    add_key_value(&stats_head,
                  "var.link.path.length","%f",
                  var_hist_array(path_length_histogram,
                                 UPPER_PATH_LENGTH) );
    add_key_value(&stats_head,
                  "std.link.path.length","%f",
                  std_dev_hist_array(path_length_histogram,
                                     UPPER_PATH_LENGTH) );

    add_key_value(&stats_head,
                  "hop.info","");  //spacer
    add_key_value(&stats_head,
                  "min.server.hop.length","%ld",
                  min_hist_array(server_hop_histogram,
                                 UPPER_SERVER_HOPS) );
    add_key_value(&stats_head,
                  "max.server.hop.length","%ld",
                  max_hist_array(server_hop_histogram,
                                 UPPER_SERVER_HOPS) );
    add_key_value(&stats_head,
                  "mean.server.hop.length","%f",
                  mean_hist_array(server_hop_histogram,
                                  UPPER_SERVER_HOPS) );
    add_key_value(&stats_head,
                  "median.server.hop.length","%f",
                  median_hist_array(server_hop_histogram,
                                    UPPER_SERVER_HOPS) );

    add_key_value(&stats_head,
                  "var.server.hop.length","%f",
                  var_hist_array(server_hop_histogram,
                                 UPPER_SERVER_HOPS) );
    add_key_value(&stats_head,
                  "std.server.hop.length","%f",
                  std_dev_hist_array(server_hop_histogram,
                                     UPPER_SERVER_HOPS) );

    //stopgap measure to avoid reporting these stats until flow
    //recording is implemented for faulty networks
#ifdef BROKEN_FLOWS_NOT_DELETED
    if(n_failures==0)
    {
#endif
        add_key_value(&stats_head,
                      "flow.info","");  //spacer

        add_key_value(&stats_head,
                      "min.flows.per.link","%ld",
                      min_hist_array(flows_histogram,
                                     upper_flows) );
        add_key_value(&stats_head,
                      "max.flows.per.link","%ld",
                      max_flow );
        add_key_value(&stats_head,
                      "mean.flows.per.link","%f",
                      mean_hist_array(flows_histogram,
                                      upper_flows) );
        add_key_value(&stats_head,
                      "median.flows.per.link","%f",
                      median_hist_array(flows_histogram,
                                        upper_flows) );
        //"mode.flows.per.link"//we don't report this because it's not
        // meaningful as a single value
        add_key_value(&stats_head,
                      "var.flows.per.link","%f",
                      var_hist_array(flows_histogram,
                                     upper_flows) );
        add_key_value(&stats_head,
                      "std.flows.per.link","%f",
                      std_dev_hist_array(flows_histogram,
                                         upper_flows) );
        add_key_value(&stats_head,
                      "connected.nonzero.flows.over.bottleneck.flow","%f",
                      (double)non_zero_path_server_pairs/max_flow );
        add_key_value(&stats_head,
                      "connected.flows.over.bottleneck.flow","%f",
                      (double)server_pairs/max_flow );
        add_key_value(&stats_head,
                      "connected.nonzero.flows.over.mean.flow","%f",
                      (double)non_zero_path_server_pairs /
                      mean_hist_array(flows_histogram,upper_flows));
        add_key_value(&stats_head,
                      "connected.flows.over.mean.flow","%f",
                      (double)server_pairs /
                      mean_hist_array(flows_histogram,upper_flows));
#ifdef BROKEN_FLOWS_NOT_DELETED
    }
#endif

#ifdef MEASURE_ROUTING_TIME
    add_key_value(&stats_head,
                  "mean.prerouting.time","%f",
                  ((float)total_prerouting_time)/prerouting_count);
    add_key_value(&stats_head,
                  "mean.hoprouting.time","%f",
                  ((float)total_hoprouting_time)/hoprouting_count);

#endif

    if(get_topo_nstats)
    {
        for (i = 0; i < get_topo_nstats(); i++)
        {
            insert_key_value(&stats_head,get_topo_key_value(i));
        }
    }

    reverse_list(&stats_head);
    print_keys(stats_head,stats_file);
    print_values(stats_head,stats_file);
    fclose(stats_file);

    print_key_value_pairs(stats_head,stdout);
    finish_stats(&stats_head);

    //AE: we will send the histograms to another file
    //now we print the three histograms, for flow length, link-path length, and server-hop length

    printf("#Server hop length histogram (server hop path length, number of occurrences)\n");
    print_hist_array(server_hop_histogram,UPPER_SERVER_HOPS,'h');
    printf("#Path length histogram (path length, number of occurrences)\n");
    print_hist_array(path_length_histogram,UPPER_PATH_LENGTH,'p');

    printf("#Flows histogram (number of flows, number of uni-directional links with this many flows)\n");
    print_hist_array(flows_histogram,upper_flows,'f');

    long **topo_hists, *topo_hists_max, *topo_hists_index, topo_nhists=0;
    long long *topo_hists_norm;
    char *topo_hists_prefix;
    const char **topo_hists_doc;
    if(get_topo_nhists)
    {
        topo_nhists=get_topo_nhists();
        topo_hists=malloc(topo_nhists*sizeof(long*));
        topo_hists_max=malloc(topo_nhists*sizeof(long));
        topo_hists_prefix=malloc(topo_nhists*sizeof(char));
        topo_hists_doc=malloc(topo_nhists*sizeof(const char*));
        topo_hists_index=malloc(topo_nhists*sizeof(long));
        topo_hists_norm=malloc(topo_nhists*sizeof(long long));
        for(i=0; i<topo_nhists; i++)
        {
            topo_hists_index[i]=0;
            topo_hists_max[i]=get_topo_hist_max(i);
            topo_hists[i]=malloc(topo_hists_max[i]*sizeof(long));
            get_topo_hist(topo_hists[i],i);
            topo_hists_prefix[i]=get_topo_hist_prefix(i);
            topo_hists_doc[i]=get_topo_hist_doc(i);
            printf("#%s.\n",topo_hists_doc[i]);
            print_hist_array(topo_hists[i],topo_hists_max[i],topo_hists_prefix[i]);
            topo_hists_norm[i]=size_hist_array(topo_hists[i],topo_hists_max[i]);
        }
    }

    snprintf(hist_filename,300,"%s/%s_hist.dat",output_dir, prefix_filename);
    hist_file = fopen(hist_filename,"w");
    if(hist_file == NULL)
    {
        perror("Unable to open stats file");
        exit(-1);
    }

#ifdef BROKEN_FLOWS_NOT_DELETED
    if(n_failures>0)
    {
#endif
        fprintf(hist_file, "#WARNING:  Flow-histogram is invalid for n.failures>0 (i.e., this output) in the current version of INRFlow\n");
#ifdef BROKEN_FLOWS_NOT_DELETED
    }
#endif

    fprintf(hist_file,
            "#Histograms for %s.\n"
            "#hi,pi,fi are indices for h, p and f, respectively. *n is normalized so that the bars add up to 1.\n"
            "#h: Server hop length histogram (server hop path length, number of occurrences)\n"
            "#p: Path length histogram (path length, number of occurrences)\n"
            "#f: Flows histogram (number of flows, number of uni-directional links with this many flows)\n",
            //	  "#If needed, omit the length 0 paths to avoid plotting the case where src=dst\n",
            hist_filename);

    //for each topo histogram, print doc for ti t
    if(topo_nhists>0)
    {
        fprintf(hist_file,
                "#Topology histograms.\n");
        for(i=0; i<topo_nhists; i++)
        {
            fprintf(hist_file,
                    "#%c: %s\n",topo_hists_prefix[i],topo_hists_doc[i]);
        }
    }

    double num_uni_dir_links = (double)size_hist_array(flows_histogram,upper_flows);
    fprintf(hist_file,"%9s %9s %6s %9s %9s %6s %9s %9s %6s",
            "hi", "h", "nh" ,"pi" ,"p", "np" ,"fi" ,"f", "nf");
    //for each topo histogram, print ti t
    for(i=0; i<topo_nhists; i++)
    {
        fprintf(hist_file," %8c%c %9c %8c%c",topo_hists_prefix[i],'i',topo_hists_prefix[i],'n',topo_hists_prefix[i]);
    }
    fprintf(hist_file,"\n");
    long hi=0,pi=0,fi=0;
    bool_t topo_flag=FALSE;
    while(hi<UPPER_SERVER_HOPS || pi < UPPER_PATH_LENGTH || fi < upper_flows || topo_flag)
    {
        hi=print_hist_next(hist_file,server_hop_histogram,
                           UPPER_SERVER_HOPS,hi,(double)connected);
        pi=print_hist_next(hist_file,path_length_histogram,
                           UPPER_PATH_LENGTH,pi,(double)connected);
        fi=print_hist_next(hist_file,flows_histogram,upper_flows,fi,num_uni_dir_links);
        //for each topo histogram, print_hist_next
        topo_flag=FALSE;
        for(i=0; i<topo_nhists; i++)
        {
            topo_hists_index[i]=print_hist_next(hist_file,topo_hists[i],
                                                topo_hists_max[i],
                                                topo_hists_index[i],
                                                (double)topo_hists_norm[i]);
            if(topo_hists_index[i]<topo_hists_max[i]) topo_flag=TRUE;
        }
        fprintf(hist_file,"\n");
    }
    fclose(hist_file);

#ifdef DEBUG
    stats_consistency_check();
#endif

    //AE: histograms were not created in here and should therefore not be
    //destroyed here.  Also, finish->destroy in a future refactoring
    finish_hist_array(&server_hop_histogram);
    finish_hist_array(&path_length_histogram);
    finish_hist_array(&flows_histogram);
    if(topo_nhists>0)
    {
        for(i=0; i<topo_nhists; i++)
        {
            finish_hist_array(&topo_hists[i]);
        }
        free(topo_hists_max);
        free(topo_hists_index);
        free(topo_hists_norm);
        free(topo_hists_prefix);
        free(topo_hists_doc);
        free(topo_hists);
    }
}

