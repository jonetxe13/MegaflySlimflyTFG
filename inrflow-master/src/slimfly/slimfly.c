/*
 * slimfly.c
 *
 *  Created on: 22 Jan 2026
 *      Author: yzy
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "../inrflow/globals.h"
#include "slimfly.h"

/*
 * Parameters (a,p,h) for the slimfly topology;
 */

static long param_p; ///< p: Number of servers connected to each switch
static long param_a; ///< a: Number of switches in each group
static long param_h; ///< h: Number of uplinks
static int param_w; ///< w: Number to generate the prime power q
static int param_q; ///< q: prime power
static int param_k; ///< k: network radix
static int param_delta_opt[3]={-1, 0, 1}; ///< delta: generator of q
static int param_delta_final; ///< delta: generator of q
static int param_l; ///< l: part of q definition
static int param_tam_gal; //tamaño de los grupos de galois


int param_gen;
int* param_x;
int* param_x2;

static long grps; ///< Total number of groups
static long switches;///< Total number of switches
static long servers;///< Total number of servers
static long ports;///< Total number of links
static long intra_ports; ///<  Total number of ports in one group connecting to other routers in the group

static long proxy_grp; ///< The switch group to use as a proxy.

static long max_paths;

static long *other_orig2map;
static long *other_map2orig;

static long ***intergroup_connections;
static long **intergroup_route;

//long * hop ;
static char network_token[14];

static char* topo_version="v0.1";
static char* topo_param_tokens[3]= {"p","a","h"};

extern char filename_params[100];
static char *routing_param_tokens[1]= {"max_paths"};
static char routing_token[30];


int* ordenar_grupo(int *grupo_x){
    int aux = 0;

    for(int i = 0; i<param_tam_gal; i++){
        for(int j = i+1; j<param_tam_gal; j++){
            if(grupo_x[i]>grupo_x[j]){
                aux = grupo_x[i];
                grupo_x[i] = grupo_x[j];
                grupo_x[j] = aux;

            }
        }
    }

    for(int i = 0; i<param_tam_gal; i++)
        printf("x[%d]=%d\n", i, grupo_x[i]);

    return grupo_x;
}

/**
 * declare the number of global connections between groups;
 */
long init_topo_slimfly(long np, long *par) {
    //Check the parameters
    if(np != 1) {
        printf("1 parameter is needed for the slimflytopology <radix>\n");
        exit(-1);
    }
    if(par[0] < 1) {
        printf("radix must be a positive number; %ld has been inserted",par[2]);
        exit(-1);
    }

    param_q = par[0]; // number of switches per group
    // check wether q is a prime of the form q = 4w+delta
    for (int j = 0; j<100; j++) {
        for (int i = 0; i<3; i++) {
            if(param_q == (4*j + param_delta_opt[i])){
                    param_delta_final = param_delta_opt[i];
                    param_l = j;
                    printf("param_delta_final: %d\n", param_delta_final);
                    break;
            }
        
        }
    }
    param_k = (3*param_q - param_delta_final)/2; //links a otros routers
    switches = 2*param_q*param_q; //number of switches
    param_p = ceil(param_k/2); // number of servers per switch
    servers = param_p * switches;
    param_a = param_q; // number of switches per group
    param_h = param_q; // number of uplinks per switch
    // printf("q: %d \n", param_q);
    // printf("k: %d \n", param_k);
    printf("switches: %d \n", switches);
    printf("p: %d \n", param_p);

    printf("servers: %d \n", servers);
	// Calculate some useful values from the parameters
    intra_ports = ((param_q- param_delta_final)/2);
    grps = param_q*2;
    param_tam_gal = (param_q - param_delta_final)/2;
    // ports = (param_q+param_p)*switches;

    //calcular el Galois Field con q
    int param_field[param_q];
    for (int i = 0; i<param_q; i++) {
        param_field[i]=i;
    }

    //mirar como buscar generadores de galois fields eficientemente
    int contador[param_q-1];

    for (int i = 2; i<param_q; i++) {
        param_gen = i;

        for (int i2 = 0; i2<param_q-1; i2++) contador[i2] = 0;

        for (int j = 0; j<param_q-1; j++) {
            int prueba_gen = (int)pow(param_gen, j);

            for (int k = 1; k<param_q; k++) {

                if((prueba_gen%param_q)==param_field[k]){
                    contador[k-1] = 1;
                    break;
                }
            }
        }
        int es_generador = 1;
        for (int i3 = 0; i3<param_q-1; i3++) 
            if(contador[i3] == 0){
                es_generador = 0;
                break;
            }
        if(es_generador) break;
    
    }

    if(param_q%4==1){
        param_x = malloc(param_tam_gal*sizeof(int));
        param_x2 = malloc(param_tam_gal*sizeof(int));
        for (int i = 0; i<param_q-1; i++) {
            if(i%2==0) param_x[i/2]=((int)pow(param_gen, i))%param_q;
            if(i%2!=0) param_x2[i/2]=((int)pow(param_gen, i))%param_q;
            // printf("param_x: %d", param_x[i/2]);
            // printf("param_x2: %d", param_x2[i/2]);
        }
    }
    else if(param_q%4 == 3){
        param_x = malloc(param_tam_gal*sizeof(int));
        param_x2 = malloc(param_tam_gal*sizeof(int));
        for (int i = 0; i<(2*param_l)-1; i+=2) {
            param_x[i/2]=((int)pow(param_gen, i))%param_q;
            param_x[param_l+i/2]=((int)pow(param_gen, 2*param_l-1+i))%param_q;
            // printf("param_x en %d: %d**%d \n", i/2, param_gen, i);
            // printf("param_x en %d: %d**%d\n", i/2, param_gen, 2*param_l-1+i);

            param_x2[i/2]=((int)pow(param_gen, i+1))%param_q;
            param_x2[param_l+i/2]=((int)pow(param_gen, 2*param_l+i))%param_q;
            // printf("param_x2 en %d: %d**%d\n", i/2, param_gen, i+1);
            // printf("param_x2 en %d: %d**%d\n", i/2, param_gen, 2*param_l+i);
            
        }
    }

    // for(int i = 0; i<param_q/2 +1; i++)
    //     printf("x[%d]=%d\n", i, param_x[i]);

    param_x = ordenar_grupo(param_x);
    param_x2 = ordenar_grupo(param_x2);

    // Boring stuff for printing and file generation
    switch(topo) {
        case SLIMFLY:
            sprintf(network_token,"slimfly");
            break;
        default:
            printf("Not a valid slimfly");
            exit(-1);
            break;
    }

    sprintf(filename_params,"p%ld_a%ld_h%ld",param_p,param_a,param_h);

  //   switch(routing) {
	 //    case DRAGONFLY_MINIMUM:
		// sprintf(routing_token,"min");
		// break;
	 //    case DRAGONFLY_VALIANT:
		// sprintf(routing_token,"valiant");
		// break;
	 //    default:
		// printf("Not a slimfly-compatible routing!");
		// exit(-1);
// }

    return 0;
}

void finish_topo_slimfly(){
    free(param_x);
    free(param_x2);
}



long get_servers_slimfly(){
    return servers;
}

long get_radix_slimfly(long n){

    if ( n < servers )
        return 1;	// This is a server
    else{
        return param_k+param_p; // This is a switch with h uplinks, p downlinks.
    }
}

tuple_t connection_slimfly(long node, long port) {
    tuple_t res={-1,-1};
    long gen_switch_id; // switch id in the general switch count
    int sw_id, sw_subgroup, grp_global, port_id; // switch (within a group), group and port id for calculating connections
    long next_grp, next_port; // group and port id of the target for calculating connections
    //


    if( node < servers ) { // The node is a server ESTO ES CORRECTO PARA slimflyTMBN
        if( port == 0 ) {
            res.node = servers + (node / param_p) ; // The server's router
            res.port = node % param_p; // The server's port number
        } // servers only have one connection
    }
    else if(node < (servers + switches)) { // the node is a switch
        gen_switch_id = node - servers; // id of the switch relative to other switches
        grp_global = (gen_switch_id/(param_q*param_q)); // id of the group relative to other groups
        if( port < param_p ) {// This is a downlink to a server 
            // res.node = grp_id*param_p*param_p + (gen_switch_id%(param_a/2))*param_p + port; //coger el server
            res.node = gen_switch_id * param_p + port;
            res.port = 0 ; // Every processor only has one port.
        }
        else if ( port < (intra_ports+param_p) ){ // Intra-group connection 
            sw_id = gen_switch_id % (param_a);
            sw_subgroup = (gen_switch_id%(param_q*param_q))/param_q;
            port_id = port - param_p;
            int indice;

            int *grupo_x = grp_global ? param_x2 : param_x;
            indice = (sw_id+grupo_x[port_id])%param_q;

            res.node = indice + servers + param_q*sw_subgroup + param_q*param_q*grp_global;

            //buscr el inverso del salto
            int inverso = (param_q-grupo_x[port_id])%param_q;
            for (int i = 0; i < intra_ports; i++) {
                if (grupo_x[i] == inverso) {
                    res.port = i + param_p;
                    break;
                }
            }
        }
        else{ //uplinks a la otra mitad
            int local_id = (gen_switch_id%(param_q*param_q));
            sw_subgroup = local_id/param_q;
            sw_id = local_id%param_q;
            port_id = port - param_p - intra_ports;

            int m, x, y, c; 
            int return_port_id;
            // printf("SWITCH ACTUAL: %ld | PUERTO ENTRADA: %ld | port_id(x o m): %d\n", gen_switch_id, port, port_id);
            if(grp_global==0){
                y =sw_id;
                x = sw_subgroup;
                m=port_id;
                c=(y-(m*x)%param_q + param_q)%param_q;
                res.node = servers + param_q*param_q + m*param_q + c;
                res.port = x + intra_ports + param_p;
                // printf("Hacia SG1 -> Nodo Destino: %ld | Puerto Retorno: %ld\n", res.node, res.port);
                // printf("res.node %ld\n", res.node);
                // printf("res.port %ld\n", res.port);
                // printf("y: %d\n", y);
                // printf("x: %d\n", x);
                // printf("c: %d\n", c);
                // printf("m: %d\n", m);
            }
            else{
                c = sw_id;
                m = sw_subgroup;
                x=port_id;
                y=(m*x + c)%param_q;
                res.node = servers + x*param_q + y;
                res.port = m + intra_ports + param_p;
                // printf("Hacia SG0 -> Nodo Destino: %ld | Puerto Retorno: %ld\n", res.node, res.port);
                // printf("res.node %ld\n", res.node);
                // printf("res.port %ld\n", res.port);
                // printf("y: %d\n", y);
                // printf("x: %d\n", x);
                // printf("c: %d\n", c);
                // printf("m: %d\n", m);
            }
        }
    }
    else {
        // Should never get here
        res.node = -1;
        res.port = -1;
    }
    return res;
}

long is_server_slimfly(long i){
    return (i < servers);
}

char * get_network_token_slimfly(){
    return network_token;
}

char * get_routing_token_slimfly(){
    return routing_token;
}

long get_swithes_slimfly(){
    return switches;
}

char *get_routing_param_tokens_slimfly(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_slimfly(){
    return topo_version;
}

char * get_topo_param_tokens_slimfly(long i){
    return topo_param_tokens[i];
}

char * get_filename_params_slimfly(){
    return filename_params;
}

long get_server_i_slimfly(long i){
    return i;
}

long get_switch_i_slimfly(long i){
    return servers + i;
}

long node_to_server_slimfly(long i){
    return i;
}

long node_to_switch_slimfly(long i){
    return i - servers;
}

long get_ports_slimfly(){
    return ports;
}

/*
 * Get the number of paths between a source and a destination.
 * @return the number of paths.
 */
long get_n_paths_routing_slimfly(long src, long dst){
    return(1);
}

long init_routing_slimfly(long src, long dst) {
    return 1;
}

void finish_route_slimfly(){

}



long route_slimfly(long current, long destination){
    long outport = 0;
    int cur_sw, dst_sw, cur_grp, dst_grp, cur_grp_global, dst_grp_global;

    dst_sw = destination/param_p;
    dst_grp_global = dst_sw/(switches/2);
    dst_grp = (dst_sw%(switches/2))/param_q;

    int dst_c, dst_m, cur_y, cur_x;

    dst_c = dst_sw%param_q;
    dst_m = dst_grp;

    if(current < servers) return 0; //el current es un server as ique puerto 0
    
    else{
        cur_sw = current - servers;
        cur_grp_global = cur_sw/(switches/2);
        cur_grp = (cur_sw%(switches/2))/param_q;

        cur_y = cur_sw%param_q;
        cur_x = cur_grp;

        int distancia = 0;
        int *grupo_x = cur_grp_global ? param_x2 : param_x;
        // for(int i = 0; i<param_q/2; i++){
        //     if(abs(cur_y-dst_c) == grupo_x[i]) distancia = 1;
        // }

        if(cur_sw == dst_sw){ //si ya estamos en el switch final que salga al server direccto
            outport = destination%param_p;
        }
        else if(cur_x==dst_m && cur_grp_global == dst_grp_global){ //mismo subgrupo (comprobar que esté bien)
            int offset=0;
            for(int i = 0; i<param_q/2 + 1; i++){ //buscar primero si con un solo salto se puede llegar
                if((cur_y+grupo_x[i])%param_q == dst_c){ 
                    offset = i;
                    outport = param_p + offset; //no creo que esté igual que en la definicion de las conexiones
                }
            }
            if(!offset){
                for(int i = 0; i<param_q/2 + 1; i++){//buscar los dos saltos que den el id del destino
                    for(int j = 0; j<param_q/2 + 1; j++){
                        if((cur_y+grupo_x[i]+grupo_x[j])%param_q == dst_c){ 
                            offset = i;
                            outport = param_p + offset; //no creo que esté igual que en la definicion de las conexiones
                        }
                    }
                }
            }
        }
        else if((cur_grp_global != dst_grp_global)){//salto al otro grupo global
            //caso en el que el salto global del switch actual ya deja a distancia 1
            
            //caso en el que el salto global del switch actual no deja a distancia 1
        }
        else{//si no saltar al otro grupo y volver

            int intermedio_m, intermedio_c;
            intermedio_m = (cur_y-dst_c)/(cur_x-dst_m);
            intermedio_c = cur_y - (intermedio_m*cur_x);


            //buscar puerto que corresponde a ese switch
        }
    }
    return outport;
}














// long route_slimfly(long current, long destination) {
//     if (current < servers) return 0; // Servidor -> Switch
//
//     long cur_sw = current - servers;
//     long dst_sw = destination / param_p;
//
//     // Si ya estamos en el switch de destino, entregar al servidor
//     if (cur_sw == dst_sw) {
//         return destination % param_p;
//     }
//
//     // --- Lógica Estilo Dragonfly adaptada a Slim Fly ---
//     // En Slim Fly tenemos 2 subgrafos (grupos) de q^2 switches cada uno
//     long switches_per_subgraph = param_q * param_q;
//     long cur_subgraph = cur_sw / switches_per_subgraph;
//     long dst_subgraph = dst_sw / switches_per_subgraph;
//
//     // CASO 1: Mismo Subgrafo (Conexiones locales X o X')
//     if (cur_subgraph == dst_subgraph) {
//         // En Slim Fly mínima, si el destino está en el mismo subgrafo
//         // y no es vecino directo, se llega a través de otro switch del mismo subgrafo.
//         // Por ahora, devolvemos un puerto local (del p en adelante)
//         // Para comprobar si funciona, enviamos al primer puerto de red disponible
//         return param_p; 
//     } 
//
//     // CASO 2: Distinto Subgrafo (Salto Global)
//     else {
//         // Aquí es donde se usa el salto global.
//         // En tu log vimos que el puerto 5 es el que conecta con el otro subgrafo.
//         // Si no hemos llegado y el destino está "al otro lado", saltamos por el puerto global.
//         return param_p + (param_k / 2); // Esto suele apuntar a los enlaces globales
//     }
// }
