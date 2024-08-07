#include "../include/matheuristic.h"

void MATsolve(TSPinst* inst, TSPenv* env) {

    double init_time = get_time();
    CPXENVptr CPLEX_env = NULL; 
    CPXLPptr CPLEX_lp = NULL;
    CPLEX_model_new(inst, &CPLEX_env, &CPLEX_lp);

    if(!strncmp(env->method,"DIVING_R", 8)) { diving(Random, CPLEX_env, CPLEX_lp, inst, env, init_time); }
    else if(!strncmp(env->method,"DIVING_W", 8)) { diving(Weighted, CPLEX_env, CPLEX_lp, inst, env, init_time); }
    else if(!strncmp(env->method,"LOCAL_BRANCH", 12)) { local_branching(CPLEX_env, CPLEX_lp, inst, env, init_time); }
    
    double final_time = get_time();
    env->time_exec = final_time - init_time;

    #if VERBOSE > 0
		print_lifespan(final_time,init_time);
	#endif
}


void diving(int strategy, CPXENVptr CPLEX_env, CPXLPptr CPLEX_lp, TSPinst* inst, TSPenv* env, const double start_time) { 

    TSPsol sol = TSPgreedy(inst, env, rand()%inst->nnodes, NULL, "", start_time);   
    TSPsol oldsol = sol;
    instance_set_solution(inst, sol.tour, sol.cost);
    CPLEX_mip_st(CPLEX_env, CPLEX_lp, inst->solution, inst->nnodes);

    int* x = calloc(inst->nnodes, sizeof(int));
    int x_size = 0;
    int percfix = 7;

    double tl = (env->time_limit - time_elapsed(start_time)) / 10;
    while(REMAIN_TIME(start_time, env)) {

        #if VERBOSE > 0
            print_state(Info, "fix %i%% of the variables\n", percfix*10);
        #endif
        x_size = arc_to_fix(strategy, x, inst, percfix, inst->nnodes/percfix);
        fix_to_model(CPLEX_env, CPLEX_lp, x, x_size);

        sol = TSPCbranchcut(inst, env, &CPLEX_env, &CPLEX_lp, tl);
        instance_set_best_sol(inst, sol);


        if(abs(sol.cost - oldsol.cost) <= EPSILON) {
            if(percfix > 4) { percfix--; }
        }
        else {
            if(percfix < 9) { percfix++; }
        }
        oldsol = sol;

        CPLEX_mip_st(CPLEX_env, CPLEX_lp, inst->solution, inst->nnodes);
        unfix_to_model(CPLEX_env, CPLEX_lp, x, x_size);
    }
}


void local_branching(CPXENVptr CPLEX_env, CPXLPptr CPLEX_lp, TSPinst* inst, TSPenv* env, const double start_time) {

    TSPsol sol = TSPgreedy(inst, env, rand()%inst->nnodes, TSPg2optb, "G2OPT_B", start_time);  
    TSPsol oldsol = sol; 
    instance_set_solution(inst, sol.tour, sol.cost);
    CPLEX_mip_st(CPLEX_env, CPLEX_lp, inst->solution, inst->nnodes);
    int k = 150;
    int deltak = 10;

    
    double tl = (env->time_limit - time_elapsed(start_time)) / 10;
    while (REMAIN_TIME(start_time, env)) {

        local_tour_costraint(CPLEX_env, CPLEX_lp, inst, k);
        sol = TSPCbranchcut(inst, env, &CPLEX_env, &CPLEX_lp, tl);

        instance_set_best_sol(inst, sol);
        print_state(Info, "SOL cost: %10.4f\n", sol.cost);
        print_state(Info, "INST cost: %10.4f\n", inst->cost);

        if(abs(sol.cost - oldsol.cost) <= EPSILON) {
            k+= deltak;
        }
        else {
            if(k > 300 ) { k-=deltak; }
        }

        print_state(-1, "K:%i\n", k);

        int nrows = CPXgetnumrows(CPLEX_env, CPLEX_lp);
        CPXdelrows(CPLEX_env, CPLEX_lp, nrows-1, nrows-1);
        CPLEX_mip_st(CPLEX_env, CPLEX_lp, inst->solution, inst->nnodes);
    }
    

}