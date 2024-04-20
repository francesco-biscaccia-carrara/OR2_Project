#ifndef __TSP_UTILS_H 

#define __TSP_UTILS_H

#define EPSILON     1e-7

#include "tsp.h"

typedef struct{
    unsigned int    i,j;
    double          delta_cost;
} cross;

typedef struct {
    double          dist;
    unsigned int    index;
} near_neighbor;

//generic functions
extern double   euc_2d(const point, const point);
extern double   delta_cost(const TSPinst*, const unsigned int, const unsigned int, const unsigned int, const unsigned int);
extern double   get_arc(const TSPinst*, const unsigned int, const unsigned int);
extern void     check_tour_cost(const TSPinst*, const int*, const double);
extern double   compute_cost(TSPinst*,const int*);

//greedy functions
extern near_neighbor get_nearest_neighbor(const TSPinst*, const unsigned int, const int*);

//G2opt functions
extern double   check_cross(const TSPinst*,const int*, const unsigned int, const unsigned int);
extern cross    find_first_cross(const TSPinst*, const int*);
extern cross    find_best_cross(const TSPinst*, const int*);

//VNS functions
extern void     kick(int*, const unsigned int);

//Display function
extern void     print_sol(const TSPinst*, const TSPenv*);
extern char*    format_arc(const TSPinst*, char*, const unsigned int, const unsigned int);
extern void     plot_log(const TSPinst*, FILE*);
extern void     plot_clog(const TSPinst*, int*, FILE*);
#endif