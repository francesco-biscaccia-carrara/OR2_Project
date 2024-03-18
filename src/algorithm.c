#include "../include/algorithm.h"
#include "../include/tsp.h"
#include "../include/utils.h"
typedef struct{
    const instance* prob;
    int* tmp_sol;
    cross* best_cross;
    int k;
} mt_data;

mt_context mt;

/// @brief check if the distance was computed before, if is not the case, compute and save the result
/// @param problem istance of the problem
/// @param i starting arc node
/// @param j ending arc node
/// @return the distance from i to j
double tsp_save_weight(instance * problem, int i, int j){
    if (i == j) return 0;

    if(!problem->edge_weights[coords_to_index(problem->nnodes,i,j)])
        problem->edge_weights[coords_to_index(problem->nnodes,i,j)] = euc_2d(&(problem->points[i]), &(problem->points[j]));
      
    return problem->edge_weights[coords_to_index(problem->nnodes,i,j)];
}

point_n_dist get_min_distance_point(int index, instance *problem, int* res) {

    double min = DBL_MAX;
    point_n_dist out = { .dist = 0.0, .index = 0};

    for(int i = 0; i < problem->nnodes; i++) {
        
        if(res[i] != 0) continue;               //if not assigned

        double dist = tsp_save_weight(problem,index,i);
        if (dist < min && dist != 0) {
            out.dist = dist;
            out.index = i;
            min = dist;
        }
    }

    return out;
}

void tsp_greedy(int index, instance* problem, cli_info* cli) {

    double cost = 0;
    int current_index = index;
    point_n_dist new_point;

    int* mem_check = calloc(problem->nnodes, sizeof(int));
    int* result = malloc(problem->nnodes * sizeof(int));

    result[0] = current_index;
    mem_check[index] = 1;

    for (int i = 1; i < problem->nnodes; i++) {  
        new_point = get_min_distance_point(result[i-1], problem, mem_check);
        
        cost += new_point.dist;
        mem_check[new_point.index] = 1;
        result[i] = new_point.index;
    }

    cost +=tsp_save_weight(problem, new_point.index, index);    
    free(mem_check);

    #if VERBOSE > 1
    printf("Partial \e[1mGREEDY\e[m solution starting from [%i]: \t%10.4f\n", index, cost);
    #endif

    if(!strncmp(cli->method,"G2OPT",5)){
        tsp_g2opt(result,&cost,problem,cli);

        #if VERBOSE > 1
        printf("Partial \e[1mG2OPT\e[m solution starting from [%i]: \t%10.4f\n", index, cost);
        #endif
    }
     
    if(cost < problem->result){
        free(problem->combination);

        problem->result = cost;
        problem->combination = result;
    }else{
        free(result);
    }
}

double check_cross(instance* problem,int* tmp_sol,int i,int j){
    int k = (j+1 == problem->nnodes) ? 0 : j+1;
    return (tsp_save_weight(problem,tmp_sol[i],tmp_sol[j])+tsp_save_weight(problem,tmp_sol[i+1],tmp_sol[k]))-(tsp_save_weight(problem,tmp_sol[i],tmp_sol[i+1])+tsp_save_weight(problem,tmp_sol[j],tmp_sol[k]));
}

cross find_best_cross(int* tmp_sol,instance* problem){
    cross best_cross = {-1,-1,INFINITY};

    for(int i=0;i<problem->nnodes-2;i++){
        for(int j=i+2;j<problem->nnodes;j++){
            if(i==0 && j+1==problem->nnodes) continue;
            double delta_cost=check_cross(problem,tmp_sol,i,j);
            if(delta_cost < best_cross.delta_cost+EPSILON){
                best_cross.i = i;
                best_cross.j = j;
                best_cross.delta_cost = delta_cost;
            }
        }
    }
    return best_cross;
}

void* find_max(void* data){
    mt_data* d = (mt_data*) data;
    instance* prob = (instance*) d->prob;
    cross * curr_cross = (cross*) d->best_cross; 

    int start = d->k * prob->nnodes/NUM_THREADS - d->k;
    int end = ((d->k)+1)* prob->nnodes/NUM_THREADS-1;
    end = end < prob->nnodes-2 ? end : prob->nnodes-2;
    cross best_cross = {-1,-1,INFINITY};
    for(int i=start;i<end;i++){
        for(int j=i+2;j<prob->nnodes;j++){
            if(i==0 && j+1==prob->nnodes) continue;
            double delta_cost=check_cross(prob,d->tmp_sol,i,j);
            if(delta_cost < best_cross.delta_cost+EPSILON){
                best_cross.i=i;
                best_cross.j=j;
                best_cross.delta_cost= delta_cost;
            }
        }
    }
    pthread_mutex_lock(&mt.mutex);
    if(best_cross.delta_cost < d->best_cross->delta_cost+EPSILON){
                d->best_cross->i = best_cross.i;
                d->best_cross->j = best_cross.j;
                d->best_cross->delta_cost = best_cross.delta_cost;
    }
    pthread_mutex_unlock(&mt.mutex);
    return NULL;
}


cross find_best_cross_mt(int* tmp_sol,const instance* problem){
    cross best_cross = {-1,-1,INFINITY};
    mt_data* data_array = malloc(NUM_THREADS * sizeof(mt_data));
    pthread_mutex_init(&mt.mutex, NULL);

    size_t nnodes = problem->nnodes;

    for (int k = 0; k < NUM_THREADS; k++) {
        data_array[k].prob = problem;
        data_array[k].tmp_sol = tmp_sol;
        data_array[k].best_cross = &best_cross;
        data_array[k].k=k;
        if(pthread_create(&mt.thread[k],NULL,find_max,(void*) &data_array[k])) exit(1);
    }

    for(int k=0;k<NUM_THREADS;k++){
        if(pthread_join(mt.thread[k],NULL)) exit(1);
    }
    pthread_mutex_destroy(&mt.mutex);
    free(data_array);
    return best_cross;
}


//TEST wheter the incumbent change its cost (just for debug)
void check_path_cost(int* tmp_sol,double tmp_cost,instance* problem){
    double cost_saved = tmp_cost;
    double computed_cost =0;
    for(int i=0;i< problem->nnodes-1;i++){
        computed_cost+=tsp_save_weight(problem,tmp_sol[i],tmp_sol[i+1]);
    }
    computed_cost+=tsp_save_weight(problem,tmp_sol[problem->nnodes-1],tmp_sol[0]);
    if (cost_saved - computed_cost > EPSILON){
        print_error("SOMETHING WRONG HAPPENS");
    }
}

void tsp_g2opt(int* tmp_sol, double* cost, instance* problem,cli_info* cli){
  char improve = 1;
  while (improve){
    cross curr_cross = (cli->thread) ? find_best_cross_mt(tmp_sol,problem): find_best_cross(tmp_sol,problem);
    if(curr_cross.delta_cost >= EPSILON){
        improve = 0;
    }else{
        reverse(tmp_sol,curr_cross.i+1,curr_cross.j);
        *cost+=curr_cross.delta_cost;
        #if VERBOSE > 2
        check_path_cost(tmp_sol,*cost,problem);
        #endif
    }
  }
}