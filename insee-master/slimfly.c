/**
 * dragonfly.c
 *
 * The dragonfly topology.
 */
#include <stdlib.h>
#include <stdio.h>

#include "spanning_tree.h"
#include "globals.h"

/*
 * Parameters (a,p,h) for the dragonfly topology;
 */

long param_q; ///< q: Prime number that defines the graph
 extern long param_p; ///< p: Number of servers connected to each switch
 extern long param_a; ///< a: Number of switches in each group
 extern long param_h; ///< h: Number of uplinks
int param_k;
int param_delta_opt[3] = {-1,0,1};
int param_delta_final;
int switches;
int servers;

 int *param_x, *param_x2;
 int param_tam_gal;

 extern long grps; ///< Total number of groups
 extern long intra_ports; ///<  Total number of ports in one group connecting to other routers in the group

long proxy_sw;

extern long escape_vcs; ///< How many VCs are needed to maintain deadlock-freedom. The remaining VCs ought to be adaptive.

extern long max_paths;

extern long *other_orig2map;
extern long *other_map2orig;

extern long ***intergroup_connections; ///< Stores the port (overall intergroup port = (next_group*param_a*param_h) + next port) connected to a given local group, switch and port; e.g., intergroup_connections[g][s][p] stores to what group is connected port p of switch s in group g. Used for several variants (Helix, Nautilux and Random).
extern long **intergroup_route;		///< Stores the output port within a group that connects to another group; e.g., intergroup_route[g0][g1] stores the group port in g0 that connects to g1. Used for several variants (Helix, Nautilux and Random).

// Función para calcular el inverso modular (a^-1 mod m)
int modInverse(int a, int m) {
    a = mod(a, m);
    for (int x = 1; x < m; x++) {
        if (mod(a * x, m) == 1)
            return x;
    }
    return 1; // Debería existir si q es primo y a != 0
}

/**
 * Calculates the neighbor for a given node and port
 * @param node. The node which is being connected.
 * @param port. The port number that is being connected.
 * @return a tuple containing the node and port it is being connected to.
 */
tuple_t connection_slimfly(long node, long port) {
    tuple_t res={-1,-1};
    long gen_switch_id; // switch id in the general switch count
    int sw_id, sw_subgroup, grp_global, port_id; // switch (within a group), group and port id for calculating connections
    long next_grp, next_port; // group and port id of the target for calculating connections
    // int switches = param_p*param_q*param_q;

    // printf("param_p=%ld, intra_ports=%ld, param_q=%ld, switches=%d\n",
    //    param_p, intra_ports, param_q, switches);
    // printf("Total ports per switch: %ld\n", param_p + intra_ports + param_q);

    if( node < nprocs) { // The node is a server ESTO ES CORRECTO PARA slimflyTMBN
        if( port == 0 ) {
            res.node = nprocs + (node / param_p) ; // The server's router
            res.port = node % param_p; // The server's port number
        } // servers only have one connection
    }
    else if(node < (nprocs + switches)) { // the node is a switch
        gen_switch_id = node - nprocs; // id of the switch relative to other switches
        grp_global = (gen_switch_id/(param_q*param_q)); // id of the group relative to other groups
        if (node == 200 && port == 7) {
        printf("DEBUG CON: nprocs=%ld, q=%d, -> TARGET=%ld\n", nprocs, param_q, res.node);
    }
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
            indice = mod((sw_id+grupo_x[port_id]),param_q);

            res.node = indice + nprocs + param_q*sw_subgroup + param_q*param_q*grp_global;

            //buscr el inverso del salto
            int inverso = mod((param_q-grupo_x[port_id]),param_q);
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
            if(grp_global==0){
                y =sw_id;
                x = sw_subgroup;
                m=port_id;
                c=mod((y-(m*x)%param_q + param_q),param_q);
                res.node = nprocs + param_q*param_q + m*param_q + c;
                res.port = x + intra_ports + param_p;
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
                y=mod((m*x + c),param_q);
                res.node = nprocs + x*param_q + y;
                res.port = m + intra_ports + param_p;
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

/**
 * Creates a dragonfly topology.
 *
 * This function defines all the links between the elements in the network.
 */
void create_slimfly(){
    long i, j, nr, np;	//neighbor router and port.
    tuple_t res;

    nnics=1;
    printf("param_x: %d, %d:\n param_x2: %d, %d\n", param_x[0], param_x[1],param_x2[0],param_x2[1]);
    // Initializating processors. Only 1 transit port plus injection queues.
    for (i=0; i<nprocs; i++){
        for (j=0; j<ninj; j++)
            inj_init_queue(&network[i].qi[j]);
        init_ports(i);
        nr = nprocs + (i / param_p); // The server's router
        np = i % param_p; // The server's port number
        network[i].nbor[0] = nr;
        network[i].nborp[0] = np;
        network[i].op_i[0] = ESCAPE;
        //		network[nr].nbor[np] = i;
        //		network[nr].nborp[np] = 0;
        //		network[nr].op_i[np] = ESCAPE;
        for (j=1; j<radix; j++){
            network[i].nbor[j] = NULL_PORT;
            network[i].nborp[j] = NULL_PORT;
            network[i].op_i[j] = ESCAPE;
        }
    }


    // Initializing switches. No injection queues needed.
    for (i=nprocs; i< NUMNODES; i++ ){
        init_ports(i);
        for (j=0; j < radix; j++ ){
            res=connection_slimfly(i,j);
            network[i].nbor[j] = res.node;			// neighbor router
            network[i].nborp[j] = res.port;			// neighbor's port
            network[i].op_i[j] = ESCAPE;
            //				network[nr].nbor[np]=i;			// neighbor router's neighbor
            //				network[nr].nborp[np]=j;		// neighbor router's neighbor's port
            //				network[nr].op_i[np] = ESCAPE;
        }
    }

}

/**
 * Generates the routing record for a dragonfly.
 *
 * @param source The source node of the packet.
 * @param destination The destination node of the packet.
 * @return The routing record needed to go from source to destination.
 */
// routing_r slimfly_rr (long source, long destination) {
//     routing_r res;
//     long src_grp=source/(param_p*param_a);
//     long dst_grp=destination/(param_p*param_a);
//     long proxy_grp=dst_grp;
//     long cur=source;
//
//     if (source == destination)
//         panic("Self-sent packet\n");
//
//     res.rr = alloc(16 * sizeof(long));
//     res.rr[15] = 0;	// Are we using a proxy? Used to decide in which virtual channel to inject the paper for the Dally mechanism
//     res.size=0;
//
//
//     while(cur!=destination){
//         // if (res.size >= 15) {
//         //     panic("Slim Fly routing loop! Check route_slimfly logic.");
//         // }
//         res.rr[res.size]=route_slimfly(cur,destination,proxy_grp);
//         cur=network[cur].nbor[res.rr[res.size]];
//         res.size++;
//     }
//
//     return res;
// }

routing_r slimfly_rr (long source, long destination) {
    routing_r res;
    long cur = source;
    long next_port;
    
    // IMPORTANTE: Inicializar proxy_grp al grupo de destino para ruteo minimal
    // Si no conoces el grupo, usa el destino directo si tu función lo permite
    long dst_sw = destination / param_p;
    long proxy_grp = dst_sw; // O el cálculo de grupo que use Slim Fly

    if (source == destination) panic("Self-sent packet\n");

    res.rr = alloc(16 * sizeof(long));
    res.size = 0;
    if(routing == VALIANT) proxy_sw = nprocs + (rand() % switches);

    while(cur != destination){
        // Seguridad: En Slim Fly, más de 4 saltos es un error de diseño
        if (res.size >= 15) {
            printf("[SLIMFLY ERROR] Bucle detectado: Origen %ld -> Destino %ld. Actualmente en Nodo %ld\n", 
                    cur, destination, cur);
            panic("Slim Fly routing loop!");
        }

        next_port = route_slimfly(cur, destination, &proxy_sw);

        res.rr[res.size] = next_port;
        cur = network[cur].nbor[next_port];
        res.size++;
    }

    return res;
}

/**
 * Routes a packet in a dragonfly.
 *
 * @param current The node where the packet is at this moment.
 * @param destination The destination node of the packet.
 * @param proxy The proxy to route through. If it is the local or destination group, the port is calculated as DIM, otherwise, the port is calculated using proxy routing.
 * @return The port to take the next hop through.
 */
long route_slimfly(long current, long destination, long *proxy) {
    long outport = -1;
    int cur_sw, dst_sw, cur_grp, dst_grp, cur_grp_global, dst_grp_global;
    // int switches = param_p*param_q*param_q;
    // int servers = nprocs ;

    if(current == *proxy) proxy_sw = *proxy;
    else proxy_sw = destination/param_p;

    dst_grp_global = proxy_sw/(switches/2);
    dst_grp = (proxy_sw%(switches/2))/param_q;

    int dst_c, dst_m, cur_y, cur_x;

    dst_c = proxy_sw%param_q;
    dst_m = dst_grp;

    // ruta[0] = swInicio;

    if(current < servers) return 0; //el current es un server as ique puerto 0
    
    else{
        cur_sw = current - servers;
        cur_grp_global = cur_sw/(switches/2);
        cur_grp = (cur_sw%(switches/2))/param_q;

        cur_y = cur_sw%param_q;
        cur_x = cur_grp;

        int distancia = 0;
        int *grupo_x = cur_grp_global ? param_x2 : param_x;
        int *grupo_x2 = cur_grp_global ? param_x : param_x2;

        if(cur_sw == proxy_sw){ //si ya estamos en el switch final que salga al server direccto
            outport = destination%param_p;
                return outport;
        }
        else if(cur_x==dst_m && cur_grp_global == dst_grp_global){ //mismo subgrupo (comprobar que esté bien)
            int directo=0;
            for(int i = 0; i<param_tam_gal; i++){ //buscar primero si con un solo salto se puede llegar
                if(mod((cur_y+grupo_x[i]),param_q) == dst_c){ 
                    directo = 1;
                    outport = param_p + i; 
                return outport;
                    break;
                }
            }
            if(!directo){
                int out = 0;
                for(int i = 0; i<param_tam_gal; i++){//buscar los dos saltos que den el id del destino
                    for(int j = 0; j<param_tam_gal; j++){
                        if(mod((cur_y+grupo_x[i]+grupo_x[j]),param_q) == dst_c){ 
                            out = 1;
                            outport = param_p + i; 
                return outport;
                            break;
                        }
                    }
                    if(out) break;
                }
            }
        }
        else if((cur_grp_global != dst_grp_global)){//salto al otro grupo global !!!!!!!!!!!!!!!!!!!!!!!!!!!!
            int directo=0;
            //caso en el que el salto global del switch actual ya deja a distancia 1
            int dst_y_salto_global;
            if(cur_grp_global == 0) //calcular la "y o c" a la que saltar
                dst_y_salto_global = mod((cur_y - dst_m*cur_x),param_q);
            else
                dst_y_salto_global = mod((cur_y + cur_x*dst_m),param_q);
                
            if(dst_y_salto_global == dst_c) {
                outport = param_p + intra_ports + dst_m;
                return outport;
                directo=1;
            }

            if(!directo){
		    for(int i = 0; i<param_tam_gal; i++){ //buscar primero si con un solo salto se puede llegar
			if(mod((dst_y_salto_global+grupo_x2[i]),param_q) == dst_c){ 
			    directo = 1;
			    outport = param_p + intra_ports + dst_m; 
                return outport;
			    break;
			}
		    }
            }

            //caso en el que el salto global del switch actual no deja a distancia 1
            if(!directo){//buscar el switch dentro del subgrupo desde el que se llega directo??
                for(int i = 0; i<param_tam_gal; i++){
                    if(cur_grp_global == 0){
                        if(mod((((cur_y+grupo_x[i])%param_q) - dst_m*cur_x),param_q) == dst_c){ 
                            outport = param_p + i; 
                return outport;
                            break;
                        }
                    }
                    else{
                        if(mod((((cur_y+grupo_x[i])%param_q) + dst_m*cur_x),param_q) == dst_c){ 
                            outport = param_p + i; 
                return outport;
                            break;
                        }

                    }
                }

            }

        }
        else if(cur_grp_global == dst_grp_global && cur_x != dst_m){//si no saltar al otro grupo y volver
            int intermedio_m, intermedio_c;
            int diff_y, diff_x, inv_x;

	    // m = (y - y') / (x - x') mod q
	    if (cur_grp_global == 0) {
		diff_y = mod(cur_y - dst_c, param_q); // dst_c es y'
		diff_x = mod(cur_x - dst_m, param_q); // dst_m es x'
		
		inv_x = modInverse(diff_x, param_q);
		intermedio_m = mod(diff_y * inv_x, param_q);
		
		// c = y - mx mod q
		intermedio_c = mod(cur_y - (intermedio_m * cur_x), param_q);
	    } 
	    else {
		diff_y = mod(dst_c - cur_y, param_q);
		diff_x = mod(cur_x - dst_m, param_q);
		
		inv_x = modInverse(diff_x, param_q);
		intermedio_m = mod(diff_y * inv_x, param_q);
		
		intermedio_c = mod(cur_y + (intermedio_m * cur_x), param_q);
	    }

            //buscar puerto que corresponde a ese switch
            outport = param_p + intra_ports + intermedio_m;
                return outport;
        }
        else{
            outport = param_p;
            printf("error, no entra en ningun if dentro del routing: x: %d; y: %d; m: %d; c: %d;\n", cur_x,cur_y,dst_m,dst_c);
        }
    }
    // if(ruta[2] != 0){
    //     printf("ruta de sw%d a sw%d", swInicio, swDestino);
    // for(int i = 0; i<5; i++){
    //     printf("%d -> ", ruta[i]);
    // }
    // printf("\n");
    // }
    return outport;
}

/**
 * Frees the data structures used by the dragonfly.
 */
void finish_slimfly(){

}
