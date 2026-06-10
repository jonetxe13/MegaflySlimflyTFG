#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

double expon(double x);
double rand_val(int seed);

int main(int argc, char *argv[]){

    FILE *fd;
    int i = 0;
    long timestamp = 0;
    int ps_aux;
    int pf_aux;
    int rem_acces;

    int io_percent = atoi(argv[1]);
    int cc_percent = atoi(argv[2]);
    long input = atol(argv[3]) * 1000000 * 8;
    long output = atol(argv[4]) * 1000000 * 8;
    float mean_size = atof(argv[5]);
    int ps = atoi(argv[6]);
    int pf = atoi(argv[7]);
    int njobs = atoi(argv[11]);
    int seed = atoi(argv[12]);
    int tasks_input;
    int tasks_ouput;
    int input_io, output_io, computation_cc, communications_cc;
    int seed_app;
    long app_size;
    long io_size;
    char filename[200];
    char stg_access[100] = "random";

    snprintf(filename, 300, "wl-cc-%d-io-%d-alloc-%s-st-%s-dam-%s-ps-%d-pf-%d-nj-%d.%d",cc_percent,io_percent,argv[8],argv[9],argv[10],ps,pf,njobs,seed); 
    srandom(seed);
    rand_val(random());

    if((fd = fopen(filename, "w")) == NULL){
        printf("Error opening file\n");
        exit(-1);
    }

    while(i < njobs){
        timestamp++;

        computation_cc = (random() % cc_percent) + 1;
        while(computation_cc == cc_percent){
            computation_cc = (random() % cc_percent) + 1;
        }
        communications_cc = cc_percent - computation_cc;
        input_io = (random() % io_percent) + 1;
        while((input_io == io_percent) || (input_io == io_percent - 1)){
            input_io = (random() % io_percent) + 1;
        }
        output_io = io_percent - input_io - 1;

        seed_app = random() % 10000;
        app_size = lround(expon(1.0 / (1.0 / mean_size))) + 63;
        
        io_size = (random() % (app_size)) + 1;
        
        if(strcmp(argv[9],"cache") == 0){
            io_size = app_size;
        }
        //else{
        //    io_size = (random() % (app_size)) + 1;
        //}
        tasks_input = (random() % (app_size)) + 1;
        tasks_ouput = (random() % (app_size)) + 1;
        
        ps_aux = (random() % ps) + 1;
        pf_aux = (random() % pf) + 1;
        rem_acces = (random() % atoi(argv[13]));
        
        fprintf(fd,"%ld markovapp_%d_%d_%d_%d_1_%d_%d_%d_%ld_%ld %d %d %s consecutive %s %s_%d_%d random_%d %s\n", timestamp, computation_cc, communications_cc, input_io, output_io, seed_app, tasks_input, tasks_ouput, input, output, app_size, io_size, argv[8], argv[9], stg_access, ps_aux, pf_aux, rem_acces, argv[10]);
        i++;
    }

    fclose(fd);


}

double rand_val(int seed){

  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  static long x;               // Random int value
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // Set the seed if argument is non-zero and then return zero
  if (seed > 0)
  {
    x = seed;
    return(0.0);
  }

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;

  // Return a random value between 0.0 and 1.0
  return((double) x / m);
}

double expon(double x){

  double z;                     // Uniform random number (0 < z < 1)
  double exp_value;             // Computed exponential value to be returned

  // Pull a uniform random number (0 < z < 1)
  do
  {
    z = rand_val(0);
  }
  while ((z == 0) || (z == 1));

  // Compute exponential random variable using inversion method
  exp_value = -x * log(z);

  return(exp_value);
}

