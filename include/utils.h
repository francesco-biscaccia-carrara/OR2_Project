#ifndef __UTILS_H 

#define __UTILS_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>  
#include <float.h> 
#include <sys/time.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <pthread.h> 
#include <unistd.h>

extern enum { Error, Warn, Info } TYPE_MESSAGE;

#define DEBUG_MODE         0
#define HANDLE_MTX         1

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct{
    int num_threads;
    pthread_mutex_t mutex;
    pthread_t* threads;
} mt_context;

extern void     print_state(int, const char*, ...);
extern int      coords_to_index(const unsigned int, const int, const int);
extern double   get_time();
extern double   time_elapsed(const double);
extern void     reverse(int*,unsigned int,unsigned int);
extern int      arrunique(const int*, const unsigned int);
extern int      ascending(const void*, const void*);
extern void     init_random();
extern char     strnin(const char*, char**, const size_t);
extern int*     cth_convert(int*, int*, const unsigned int);
extern int      get_subset_array(int*, int*, int);

//log utils
extern void             format_csv_line(FILE*, const double*, const unsigned int);
extern void             print_lifespan(const double, const double);
extern mt_context*      new_mt_context(int,char);
extern void             run_job(mt_context*,void* (*funct)(void*) ,void* );
extern void             delete_mt_context(mt_context*,char);
extern void             run_mt_context(mt_context* ,int ,void* (*funct)(void*) ,void* );
#endif