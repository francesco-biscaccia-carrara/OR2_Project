#include "../include/matheuristic.h"

void MATsolve(TSPinst* inst, TSPenv* env) {

    double init_time = get_time();
    CPXENVptr CPLEX_env = NULL; 
    CPXLPptr CPLEX_lp = NULL;
    CPLEX_model_new(inst, &CPLEX_env, &CPLEX_lp);

    if(!strncmp(env->method,"DIVING_R", 8)) { diving(Random, CPLEX_env, CPLEX_lp, inst, env, init_time); }
    else if(!strncmp(env->method,"DIVING_W", 8)) { diving(Weighted, CPLEX_env, CPLEX_lp, inst, env, init_time); }
    //else if(!strncmp(env->method,"DIVING_P", 8)) { diving(Probably, CPLEX_env, CPLEX_lp, inst, env, init_time); }
    
    else if(!strncmp(env->method,"LOCALBRANCH", 11)) { local_branching(CPLEX_env, CPLEX_lp, inst, env, init_time); }
    
    double final_time = get_time();
    env->time_exec = final_time - init_time;

    #if VERBOSE > 0
		print_lifespan(final_time,init_time);
	#endif
}


void diving(int strategy, CPXENVptr CPLEX_env, CPXLPptr CPLEX_lp, TSPinst* inst, TSPenv* env, const double start_time) { 

    TSPsol sol = TSPgreedy(inst, rand()%inst->nnodes, NULL, "");   
    TSPsol oldsol = sol;
    instance_set_solution(inst, sol.tour, sol.cost);
    CPLEX_post_heur(&CPLEX_env, &CPLEX_lp, inst->solution, inst->nnodes);

    int* x = calloc(inst->nnodes, sizeof(int));
    int x_size = 0;
    int percfix = 7;

    while(REMAIN_TIME(start_time, env)) {

        #if VERBOSE > 0
            print_state(Info, "fix %i%% of the variables\n", percfix*10);
        #endif
        x_size = arc_to_fix(strategy, x, inst, percfix, inst->nnodes/percfix);
        fix_to_model(CPLEX_env, CPLEX_lp, x, x_size);

        sol = TSPCbranchcut(inst, env, &CPLEX_env, &CPLEX_lp, start_time);
        instance_set_best_sol(inst, sol);


        if(abs(sol.cost - oldsol.cost) <= EPSILON) {
            if(percfix > 4) { percfix--; }
        }
        else {
            if(percfix < 9) { percfix++; }
        }
        oldsol = sol;

        CPLEX_edit_post_heur(&CPLEX_env, &CPLEX_lp, inst->solution, inst->nnodes);
        unfix_to_model(CPLEX_env, CPLEX_lp, x, x_size);
    }
}


void local_branching(CPXENVptr CPLEX_env, CPXLPptr CPLEX_lp, TSPinst* inst, TSPenv* env, const double start_time) {

    // \sum xe >= n-k
    TSPsol sol = TSPgreedy(inst, rand()%inst->nnodes, TSPg2optb, "G2OPT_B");   
    instance_set_solution(inst, sol.tour, sol.cost);
    CPLEX_post_heur(&CPLEX_env, &CPLEX_lp, inst->solution, inst->nnodes);

    int k = 20;
    int del_k = 10;
    int* limit = calloc(inst->nnodes, sizeof( int ));
    double old_sol = inst->cost;
    double new_sol = inst->cost;

    //NOTE:
    //  1. k non troppo grosso = 1mln di variabili
    //  2  fornire soluzione decente
    
    for (int start_time = get_time(); time_elapsed(start_time) < env->time_limit;) {
        
        //Aggiungi Vicolo
        for(int i = 0; i < inst->nnodes-1; i++) {
            limit[i] = coords_to_index(inst->nnodes, inst->solution[i], inst->solution[i+1]);
        } 
        limit[inst->nnodes] = coords_to_index(inst->nnodes, inst->solution[inst->nnodes - 1], inst->solution[0]);
        
        //Solve
        TSPCbranchcut(inst, env, &CPLEX_env, &CPLEX_lp, start_time);
        new_sol = inst->cost;

        //Rimuovi Vincolo
        //CPX_delete  RO

    }
    

}