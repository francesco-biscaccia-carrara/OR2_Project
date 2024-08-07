#include "include/tsp_solver.h"
#include "include/tsp_exact.h"
#include "include/matheuristic.h"


int main(int argc, char **argv) {
    TSPenv* env = environment_new_cli(argv, argc);
    TSPinst* inst = instance_new_env(env);

    char* cplex_func[] = { "BENDERS", "BRANCH_CUT" };
    char* mathe_func[] = { "DIVING_R", /*"DIVING_P",*/ "DIVING_W", "LOCAL_BRANCH" };

    #if VERBOSE > 0
        if(env->perf_v) print_state(Error,"Set VERBOSE 0 to run the test\n");
    #endif
    
    double init_time = get_time();
    if(strnin(env->method, mathe_func, 3)) { 
        MATsolve(inst, env);
        TSPg2optb(inst, env, init_time, inst->solution, &(inst->cost));    
    }
    else if(strnin(env->method, cplex_func, 2)) {
        TSPCsolve(inst,env);
        TSPg2optb(inst, env, init_time, inst->solution, &(inst->cost));
    }
	else TSPsolve(inst, env);

    #if VERBOSE > 2
        check_tour_cost(inst, inst->solution, inst->cost);
    #endif


    if(env->perf_v) {
        printf("%10.4f, %10.4f", env->time_exec, inst->cost);
    }
    else {
        print_sol(inst, env);
        FILE* f = fopen("plot/input/test.txt", "w");
        plot_log(inst, f);
        fclose(f);
        system("cd plot && python3 plot_solution.py");
    }
    
    instance_delete(inst);
    environment_delete(env);
    return 0;
}