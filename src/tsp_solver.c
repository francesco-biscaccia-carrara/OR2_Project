#include "../include/tsp_solver.h"

#define BEST_TENURE 2
#define BEST_KICK 1

static mt_context* GREEDY_MT_CTX;

static void* greedy_job(void* userhandle){
    mt_greedy_pars pars = *(mt_greedy_pars*) userhandle;

    TSPsol my_min = { .cost = INFINITY, .tour = NULL };

    int my_id;
    for(my_id=0;(pthread_self() != GREEDY_MT_CTX->threads[my_id]) && my_id< GREEDY_MT_CTX->num_threads;my_id++);

    int load = pars.mt_inst->nnodes/GREEDY_MT_CTX->num_threads;
    int end = (my_id != GREEDY_MT_CTX->num_threads-1) ? (my_id+1) * load: pars.mt_inst->nnodes;

    for(int i = my_id * load; i < end && REMAIN_TIME(pars.mt_init_time,pars.mt_env); i++){
        TSPsol tmp = TSPgreedy(pars.mt_inst, pars.mt_env, i, pars.mt_opt_fun, pars.mt_env->method, pars.mt_init_time);
        if(tmp.cost < my_min.cost) { my_min = tmp; }
    }

    pthread_mutex_lock(&GREEDY_MT_CTX->mutex);
        if(my_min.cost < pars.mt_greedy_sol->cost)  *pars.mt_greedy_sol = my_min;    
    pthread_mutex_unlock(&GREEDY_MT_CTX->mutex);
    return NULL;
}


/// @brief Solve an instance of TSP with an heuristic approach
/// @param inst instance of TSPinst
/// @param env instance of TSPenv
void TSPsolve(TSPinst* inst, TSPenv* env) {
    char* null_func[] = {"GREEDY", "TABU_R", "VNS"};
    char* optb_func[] = {"G2OPT_B", "TABU_B"};
    char* optf_func[] = {"G2OPT_F"};
    void* opt_func;


    //set_improvement function
    if(strnin(env->method, null_func, 3)) { opt_func = NULL; }
    else if(strnin(env->method, optf_func, 1)) { opt_func = TSPg2opt; }
    else if(strnin(env->method, optb_func, 2)) { opt_func = TSPg2optb; }
    else { print_state(Error, "No function with alias"); }


    #if VERBOSE > 1
        print_state(Info,"Multithreading on %d threads\n",(int) log2(inst->nnodes*(inst->nnodes-1)/2));
    #endif
    
    double init_time = get_time();

    TSPsol min = { .cost = INFINITY, .tour = NULL };

    mt_greedy_pars greedy_par ={.mt_inst=inst,
                                .mt_opt_fun=opt_func,
                                .mt_init_time=init_time,
                                .mt_env=env,
                                .mt_greedy_sol=&min};

    GREEDY_MT_CTX = new_mt_context((int) log2(inst->nnodes*(inst->nnodes-1)/2),HANDLE_MTX);
    run_job(GREEDY_MT_CTX,greedy_job,&greedy_par);
    delete_mt_context(GREEDY_MT_CTX,HANDLE_MTX);
    

    instance_set_solution(inst, min.tour, min.cost);

    
    char* vns_func[] = {"VNS"};
    char* tabu_func[] = {"TABU_R", "TABU_B"};
    if(strnin(env->method, vns_func, 1)) { min = TSPvns(inst, env, init_time); }
    else if(strnin(env->method, tabu_func, 2)) { min = TSPtabu(inst, env, init_time); }
    instance_set_best_sol(inst, min);

    
    double final_time = get_time();
    env->time_exec = final_time - init_time;

    #if VERBOSE > 0
		print_lifespan(final_time,init_time);
	#endif
}


/// @brief find a solution to TSP using gredy approach
/// @param inst instance of TSPinst
/// @param intial_node intial node
/// @param tsp_func improvement function
TSPsol TSPgreedy(const TSPinst* inst, const TSPenv* env, const unsigned int intial_node, void(tsp_func)(const TSPinst*, const TSPenv*,double, int*, double*), char* func_name, double init_time) {

    TSPsol out = { .cost = 0.0, .tour = malloc(inst->nnodes * sizeof(int)) };

    int current_index = intial_node;
    char used_node[inst->nnodes];
    bzero( used_node, inst->nnodes * sizeof(char) );
    used_node[intial_node] = 1;
    out.tour[0] = current_index;

    for (int i = 1; i < inst->nnodes; i++) {  
        near_neighbor new_point = get_nearest_neighbor(inst, out.tour[i-1], used_node);

        out.cost += new_point.dist;
        used_node[new_point.index] = 1;
        out.tour[i] = new_point.index;
    }
    out.cost += get_arc(inst, out.tour[inst->nnodes-1], intial_node);

    #if VERBOSE > 1
    printf("Partial \e[1m%7s\e[m solution starting from [%i]: \t%10.4f\n","GREEDY" , intial_node, out.cost);
    #endif

    if(tsp_func != NULL) {
        tsp_func(inst,env, init_time, out.tour, &out.cost);

        #if VERBOSE > 1
        printf("Partial \e[1m%7s\e[m solution starting from [%i]: \t%10.4f\n", func_name, intial_node, out.cost);
        #endif
    }

    return out;
}


/// @brief execute G2Opt using first cross policy 
/// @param inst instance of TSPinst 
/// @param tour hamiltionian circuit
/// @param cost cost of path
void TSPg2opt(const TSPinst* inst,const TSPenv* env, double init_time, int* tour, double* cost) {

    while (REMAIN_TIME(init_time, env)) {
        cross curr_cross = find_first_cross(inst, tour);
        if(curr_cross.delta_cost >= -EPSILON) return;
        
        reverse(tour,curr_cross.i+1,curr_cross.j);
        *cost+=curr_cross.delta_cost;

        #if VERBOSE > 2
            check_tour_cost(inst, tour, *cost);
        #endif
    }
}


/// @brief execute G2Opt using best cross policy 
/// @param inst instance of TSPinst 
/// @param tour hamiltionian circuit
/// @param cost cost of path
void TSPg2optb(const TSPinst* inst,const TSPenv* env, double init_time, int* tour, double* cost) {

    while (REMAIN_TIME(init_time, env)) {
        cross curr_cross = find_best_cross(inst, tour);
        if(curr_cross.delta_cost >= -EPSILON) return;
        
        reverse(tour,curr_cross.i+1,curr_cross.j);
        *cost+=curr_cross.delta_cost;
            
        #if VERBOSE > 2
            check_tour_cost(inst, tour, *cost);
        #endif
    }
}


/// @brief Use tabu seach to solve TSP in heuristic way
/// @param inst instance of TSPinst
/// @param env instance of TSPenv
/// @param init_time initial time
TSPsol TSPtabu(TSPinst* inst, const TSPenv* env, const double init_time) {

    TSPsol out = { .cost = inst->cost, .tour = malloc(inst->nnodes * sizeof(int)) };
    
    int tabu_index = 0; 
    //if(env->tabu_par<=0) print_state(Error,"not valid parameter!");
    //int size_ratio = env->tabu_par;
    int size_ratio = BEST_TENURE;
    int tabu_size = inst->nnodes / size_ratio;
    cross tabu[tabu_size];

    double cost = inst->cost;
    int tmp_sol[inst->nnodes];
    memcpy(tmp_sol, inst->solution, inst->nnodes * sizeof(inst->solution[0]));

    while (REMAIN_TIME(init_time, env))
    {
        cross move = find_best_t_cross(inst, tmp_sol, tabu, tabu_size);
        reverse(tmp_sol, move.i + 1, move.j);
        cost += move.delta_cost;

        if(move.delta_cost >= EPSILON) {
            tabu[tabu_index % tabu_size] = move;
            tabu_index++;
        }
        else if(cost < out.cost - EPSILON) {
            out.cost = cost;
            memcpy(out.tour, tmp_sol, inst->nnodes * sizeof(int));

            #if VERBOSE > 0
                print_state(Info, "%3s -- New best cost:\t%10.4f\n",env->method, out.cost);
            #endif
            #if VERBOSE > 2
            check_tour_cost(inst, tmp_sol, cost);
            #endif
        }
    }   

    return out;
}


/// @brief execute VNS algorithm to improve TSP instance
/// @param inst instance of TSPinst 
/// @param env instance of TSPenv
/// @param init_time initial time
TSPsol TSPvns(TSPinst* inst, const TSPenv* env, const double init_time) { 

    TSPsol out = { .cost = inst->cost, .tour = malloc(inst->nnodes * sizeof(int)) };

    double cost = inst->cost;
    int tmp_sol[inst->nnodes];
    //if(env->vns_par<=0) print_state(Error,"not valid parameter!");
    
    memcpy(tmp_sol, inst->solution, inst->nnodes * sizeof(inst->solution[0]));
    int kick_size = 1;

    while (REMAIN_TIME(init_time, env)) {
        TSPg2optb(inst, env, init_time, tmp_sol, &cost);

        if(cost < out.cost - EPSILON) {
            out.cost = cost;
            memcpy(out.tour, tmp_sol, inst->nnodes * sizeof(int));
            kick_size = (kick_size <= 1) ? 1 : kick_size--;
            
            #if VERBOSE > 0
                print_state(Info, "%3s -- New best cost:\t%10.4f\n",env->method, out.cost);
            #endif
        }
        else kick_size = (kick_size <= BEST_KICK) ? BEST_KICK : kick_size++;

        for(int i = 0; i < kick_size; i++) cost += kick(inst, tmp_sol, inst->nnodes);
        #if VERBOSE > 2
            check_tour_cost(inst, tmp_sol, cost);
        #endif
    }

    return out;
}