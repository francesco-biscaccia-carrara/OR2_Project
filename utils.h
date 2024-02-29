#ifndef __UTILS_H 

#define __UTILS_H

#include "tsp.h"

void free_instance(instance * inst);

void print_error(const char *err);

uint64_t get_time();

void read_tsp_file(instance * inst);

void parse_cli(int argc, char** argv, instance *inst);

#endif