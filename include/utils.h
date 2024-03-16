#ifndef __UTILS_H 

#define __UTILS_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>  
#include <float.h> 
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>

#define SQUARE(x)   (x*x)
//#define NINT(x)     ((int) x + 0.5)

typedef struct {
    double x;
    double y;
} point;

typedef struct {
    size_t nnodes;
    uint32_t random_seed;
    char file_name[60];
    char method[20];
    uint64_t time_limit;
    int thread;
} cli_info;

typedef struct{
    int i,j;
    double delta_cost;
} cross;

#define NUM_THREADS 16 //2 * core on my cpu
typedef struct{
    pthread_mutex_t mutex;
    pthread_t thread[NUM_THREADS];
} mt_context;


extern void print_error(const char *err);
extern uint64_t get_time();
extern void help_info();
extern int coords_to_index(uint32_t n,int i,int j);
extern double euc_2d(point* a, point* b);
extern void reverse(int* solution, int i, int j);
#endif