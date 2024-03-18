#include "../include/utils.h"

#define INDEX(n,i,j) ((i*n-(i-1)*(i)/2) + (j-i-1)-i)

void print_error(const char *err){
    printf("\n\x1b[31mERROR: %s\x1b[0m\n", err); 
    fflush(NULL); 
    exit(1);
} 

uint64_t get_time(){
    return (uint64_t) time(NULL);
}

void help_info(){
    printf("\e[1mTo set the parameters properly you have to execute tsp and add:\e[m");
    printf("\n '-in / -f / -file <filename.tsp>' to specity the input file; ");
    printf("\n '-tl / -max_time <time_dbl>' to specity the max execution time (int value);");
    printf("\n '-n / -n_nodes <num_nodes_int>' to specify the number of nodes in the TSP instance (int value);");
    printf("\n '-algo / -method / -alg <method>' to specify the method to solve the TSP instance;");
    printf("\n\tImplemented method: GREEDY = greedy search, G2OPT = greedy + 2opt");
    printf("\n '-seed / -rnd_seed <seed>' to specity the random seed (int value);");
    printf("\n '-help / --help / -h' to get help.");
    printf("\n\nNOTICE: you can insert only .tsp file or random seed and number of nodes, NOT BOTH!\n");
}

/// @brief transform 2d coordinate for a triangular matrix in 1d array
/// @param n number of rows
/// @param i row
/// @param j column
/// @return index where the desired value is stored
int coords_to_index(uint32_t n, int i, int j){
    return i<j ? INDEX(n,i,j) : INDEX(n,j,i);
}

double euc_2d(point* a, point* b) {
    double dx = b->x - a->x;
    double dy = b->y - a->y; 

    return sqrt(SQUARE(dx) + SQUARE(dy));
}

void reverse(int* solution, int i,int j){
    while(i<j){
        int tmp = solution[i];
        solution[i]=solution[j];
        solution[j]=tmp;
        i++;j--;
    }
}

void init_mt_context(dyn_mt_context* ctx,int num_threads){
    ctx->num_threads=num_threads;
    ctx->threads = (pthread_t*) malloc(num_threads*sizeof(pthread_t));
    pthread_mutex_init(&ctx->mutex, NULL);
}

void assign_task(dyn_mt_context* ctx,int th_i,void* (*funct)(void*) ,void* args){
    if(pthread_create(&ctx->threads[th_i],NULL,funct,args)) print_error("bad tasks assignement!");
}

void delete_mt_context(dyn_mt_context* ctx){
    for(int k=0;k<ctx->num_threads;k++) if(pthread_join(ctx->threads[k],NULL))  print_error("bad join of threads!");
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx->threads);
}

