#ifndef __ALGO_H 

#define __ALGO_H

#include "tsp.h"

extern void tsp_greedy(int index, instance* problem, cli_info* cli);
extern void tsp_g2opt(int* tmp_sol, double* cost, instance* problem,cli_info* cli);
#endif