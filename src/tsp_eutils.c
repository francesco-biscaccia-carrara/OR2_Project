#include "../include/tsp_eutils.h"

/// @brief Create a CPLEX problem (env,lp) from a TSP instance
/// @param inst TSPinst instance pointer
/// @param env CPLEX environment pointer
/// @param lp CPLEX model pointer
void CPLEX_model_new(TSPinst* inst, CPXENVptr* env, CPXLPptr* lp) {
	//Env and empty model created
	int error;
	*env = CPXopenCPLEX(&error);
	*lp  = CPXcreateprob(*env, &error, "TSP"); 
	if(error) print_state(Error, "model not created");

	int izero = 0;
	char binary = 'B'; 
    double lb = 0.0;
	double ub = 1.0;

	char **cname = (char **) calloc(1, sizeof(char *));	
	cname[0] = (char *) calloc(100, sizeof(char));

	// add binary var.s x(i,j) for i < j  
	for ( int i = 0; i < inst->nnodes; i++ ){
		for ( int j = i+1; j < inst->nnodes; j++ ){

			sprintf(cname[0], "x(%d,%d)", i+1,j+1);  
			double obj = get_arc(inst,i,j);

			if ( CPXnewcols(*env, *lp, 1, &obj, &lb, &ub, &binary, cname) ) print_state(Error, "wrong CPXnewcols on x var.s");
        	if ( CPXgetnumcols(*env,*lp)-1 != coords_to_index(inst->nnodes,i,j) ) print_state(Error, "wrong position for x var.s");
		}
	} 

	int *index = (int *) malloc(inst->nnodes * sizeof(int));
	double *value = (double *) malloc(inst->nnodes * sizeof(double));  
	
	// add the degree constraints
	for ( int h = 0; h < inst->nnodes; h++ )  {
		double rhs = 2.0;
		char sense = 'E';                     // 'E' for equality constraint 
		sprintf(cname[0], "degree(%d)", h+1); 
		int nnz = 0;
		for ( int i = 0; i < inst->nnodes; i++ ){
			if ( i == h ) continue;
			index[nnz] = coords_to_index(inst->nnodes,h, i);
			value[nnz] = 1.0;
			nnz++;
		}
		
		if (CPXaddrows(*env, *lp, 0, 1, nnz, &rhs, &sense, &izero, index, value, NULL, &cname[0]) ) print_state(Error, " wrong CPXaddrows [degree]");
	} 

    free(value);
    free(index);
	free(cname[0]);
	free(cname);
}


/// @brief Delete a CPLEX problem (env,lp) 
/// @param env CPLEX environment pointer
/// @param lp CPLEX model pointer
void CPLEX_model_delete(CPXENVptr* env, CPXLPptr* lp) {
	CPXfreeprob(*env, lp);
	CPXcloseCPLEX(env); 
}


/// @brief Set CPLEX log on
/// @param env CPLEX environment pointer
/// @param env TSPenv instance pointer
void CPLEX_log(CPXENVptr* env,const TSPenv* tsp_env){
	CPXsetdblparam(*env, CPX_PARAM_SCRIND, CPX_OFF);
	CPXsetintparam(*env, CPX_PARAM_CLONELOG, -1);
    char cplex_log_file[100];
    sprintf(cplex_log_file, "log/n_%u-%d-%s.log", tsp_env->random_seed, tsp_env->nnodes,tsp_env->method);
    if ( CPXsetlogfilename(*env, cplex_log_file, "w") ) print_state(Error, "CPXsetlogfilename error.\n");
}


/// @brief Convert the solution saved on inst->solution to CPX format
/// @param inst TSPinst pointer
/// @param index array of indeces
/// @param value array of non-zeros
static inline void CPLEX_sol_from_inst(const unsigned int nnodes, const int* solution,int* index, double* value) {
		for(int i = 0; i < nnodes-1; i++){
			index[i] = coords_to_index(nnodes,solution[i],solution[i+1]);
			value[i] = 1.0;
		}
		index[nnodes-1] = coords_to_index(nnodes,solution[nnodes-1],solution[0]);
		value[nnodes-1] = 1.0;
}


/// @brief Decompose the solution in the xstar format into n-component format
/// @param xstar solution in xstar format pointer
/// @param nnodes numbers of nodes
/// @param succ array of successor necessary to store the solution
/// @param comp array that associate a number from 1 to n-component for each node
/// @param ncomp number of component pointer
void decompose_solution(const double *xstar, const unsigned int nnodes, int *succ, int *comp, int *ncomp, int* compstarts){   
	#if VERBOSE > 2
		int *degree = (int *) calloc(nnodes, sizeof(int));
		for ( int i = 0; i < nnodes; i++ )
		{
			for ( int j = i+1; j < nnodes; j++ )
			{
				int k = coords_to_index(nnodes, i,j);
				if ( fabs(xstar[k]) > EPSILON && fabs(xstar[k]-1.0) > EPSILON ) print_state(Error, " wrong xstar in decompose_sol()");
				if ( xstar[k] > 0.5 ) 
				{
					++degree[i];
					++degree[j];
				}
			}
		}
		for ( int i = 0; i < nnodes; i++ )
		{
			if ( degree[i] != 2 ) print_state(Error, "wrong degree in decompose_sol()");
		}	
		free(degree);
	#endif

    int nstart = 0;

	*ncomp = 0;
	for ( int i = 0; i < nnodes; i++ ) {
		succ[i] = -1;
		comp[i] = -1;
	}
	
	for ( int start = 0; start < nnodes; start++ ){
		if ( comp[start] >= 0 ) continue;  // node "start" was already visited, just skip it
		(*ncomp)++;
		int i = start;

        if(compstarts != NULL)  compstarts[nstart++] = i;

		int done = 0;
		while ( !done ){
			comp[i] = *ncomp;
			done = 1;
			for ( int j = 0; j < nnodes; j++ ){
				if ( i != j && xstar[coords_to_index(nnodes,i,j)] > 0.5 && comp[j] == -1 ) {// the edge [i,j] is selected in xstar and j was not visited before
					succ[i] = j;
					i = j;
					done = 0;
					break;
				}
			}
		}	
		succ[i] = start;
	}
}


/// @brief Post an heuristic solution inside CPLEX model
/// @param succ TSPinst solution
/// @param nnodes number of nodes
/// @param env CPLEX environment pointer
/// @param lp CPLEX model pointer
void CPLEX_post_heur(CPXENVptr env, CPXLPptr lp, int* succ, const unsigned int nnodes) {

	int start_index = 0;
	int effort_level = CPX_MIPSTART_NOCHECK;
	int* index = (int*) calloc(nnodes,sizeof(int));
	double* value = (double*) calloc(nnodes,sizeof(double));

	CPLEX_sol_from_inst(nnodes,succ,index,value);

	if (CPXaddmipstarts(env, lp, 1, nnodes, &start_index, index, value, &effort_level, NULL)) print_state(Error, "CPXaddmipstarts() error");	
	
	free(index);
	free(value);
}

void CPLEX_edit_post_heur(CPXENVptr* env, CPXLPptr* lp, int* succ, const unsigned int nnodes) {
	
	int start_index = 0;
	int mipindex = 0;
	int effort_level = CPX_MIPSTART_NOCHECK;
	int* index = (int*) calloc(nnodes,sizeof(int));
	double* value = (double*) calloc(nnodes,sizeof(double));

	CPLEX_sol_from_inst(nnodes,succ,index,value);

	if (CPXchgmipstarts(*env, *lp, 1, &mipindex, nnodes, &start_index, index, value, &effort_level)) print_state(Error, "CPXaddmipstarts() error");

	free(index);
	free(value);
}


/*======================================================================*/

static inline void add_SEC_cut(int k,int* nz, double* rh, int* index, double* value, const int*comp, int nnodes) {
	for(int i = 0;i<nnodes;i++){
		if(comp[i]!=k) continue;
		(*rh)++;
		for(int j =i+1;j < nnodes;j++){
			if(comp[j]!=k) continue;
			index[*nz]=coords_to_index(nnodes,i,j);
			value[*nz]=1.0;
			(*nz)++;
		}
	}
}


/// @brief Add SECs as new constraints in the CPLEX model
/// @param env CPLEX environment pointer
/// @param lp CPLEX model pointer
/// @param nnodes number of nodes
/// @param ncomp number of component
/// @param comp array that associate a number from 1 to n-component for each node
void add_SEC_mdl(CPXCENVptr env, CPXLPptr lp,const int* comp, const unsigned int ncomp, const unsigned int nnodes, int* succ, int* nstarts){

	if(ncomp==1) print_state(Error, "no sec needed for 1 comp!");

	int* index = (int*) calloc((nnodes*(nnodes-1))/2,sizeof(int));
	double* value = (double*) calloc((nnodes*(nnodes-1))/2,sizeof(double));
	char sense ='L';
	int start_index = 0;
	

	int* out = calloc(nnodes, sizeof(int));
	for(int k=1;k<=ncomp;k++) {
		int nnz=0;
		double rhs=-1.0;
		int ssize = get_subset_array(out, succ, nstarts[k-1]);

        for(int i = 0; i < ssize; i++) {
			rhs++;
			for(int j = i+1; j < ssize; j++) {
				index[nnz]=coords_to_index(nnodes,out[i],out[j]);
				value[nnz]=1.0;
				nnz++;
			}
		}
	
		//add_SEC_cut(k, &nnz, &rhs, index, value, comp, nnodes);
		if( CPXaddrows(env,lp,0,1,nnz,&rhs,&sense,&start_index,index,value,NULL,NULL)) print_state(Error, "CPXaddrows() error");
	}
	free(index);
	free(value);
}


int add_SEC_int(CPXCALLBACKCONTEXTptr context,TSPinst inst){
	  	
	int ncols = inst.nnodes*(inst.nnodes-1)/2;
	double* xstar = (double*) malloc(ncols * sizeof(double));  
	double objval = CPX_INFBOUND; 

	if (CPXcallbackgetcandidatepoint(context, xstar, 0, ncols-1, &objval)) print_state(Error, "CPXcallbackgetcandidatepoint error");

	int *succ = calloc(inst.nnodes,sizeof(int));
	int *comp = calloc(inst.nnodes,sizeof(int));
    int *nstart = calloc(inst.nnodes/2, sizeof(int)); 
	int ncomp;

	decompose_solution(xstar,inst.nnodes,succ,comp,&ncomp, nstart);
	free(xstar);

	if (ncomp == 1) {
		free(succ);
		free(comp);
		
		#if VERBOSE > 0
			double incumbent = CPX_INFBOUND;
			double l_bound = CPX_INFBOUND;
			CPXcallbackgetinfodbl(context,CPXCALLBACKINFO_BEST_SOL,&incumbent);
			CPXcallbackgetinfodbl(context,CPXCALLBACKINFO_BEST_BND,&l_bound);
			printf("\e[1mBRANCH & CUT\e[m new Feasible Solution - Incumbent: %20.4f\tLower-Bound: %20.4f\tInt.Gap: %1.2f%% \n",incumbent,l_bound,(1-l_bound/incumbent)*100);
		#endif

		return 0;
	}
	

	//Add sec section
	int* index = calloc(ncols,sizeof(int));
	double* value = calloc(ncols,sizeof(double));
	
	char sense ='L';
	int start_index = 0;

	#if VERBOSE > 1
		printf("\e[1mBRANCH & CUT\e[m \t%4d \e[3mCANDIDATE cuts\e[m found\n",ncomp);
	#endif


	int* out = calloc(inst.nnodes, sizeof(int));
	for(int k=1;k<=ncomp;k++) {
		int nnz=0;
		double rhs = -1.0;
		//add_SEC_cut(k, &nnz, &rhs, index, value, comp, inst.nnodes); 

		int ssize = get_subset_array(out, succ, nstart[k-1]);

		for(int i = 0; i < ssize; i++) {
			rhs++;
			for(int j = i+1; j < ssize; j++) {
				index[nnz]=coords_to_index(inst.nnodes,out[i],out[j]);
				value[nnz]=1.0;
				nnz++;
			}
		}
	
		if (CPXcallbackrejectcandidate(context, 1, nnz, &rhs, &sense, &start_index, index, value) ) print_state(Error, "CPXcallbackrejectcandidate() error"); 
	
	}
	free(index);
	free(value);

	free(succ);
	free(comp);
	return 0;
}


int add_cut_CPLEX(double cut_value, int cut_nnodes, int* cut_index_nodes, void* userhandle){
	
	cut_par cut_pars = *(cut_par*) userhandle;

	int* index = calloc(cut_nnodes*(cut_nnodes-1)/2,sizeof(int));
	double* value = calloc(cut_nnodes*(cut_nnodes-1)/2,sizeof(double));

	int izero = 0;
	int purgeable = CPX_USECUT_FILTER;
	int local = 0;
	double rhs = -1.0;
	char sense = 'L';
	int nnz = 0;

	for(int i = 0; i<cut_nnodes;i++){
		rhs++;
		for(int j =i+1;j < cut_nnodes; j++){
			index[nnz]=coords_to_index(cut_pars.nnodes,cut_index_nodes[i],cut_index_nodes[j]);
			value[nnz]=1.0;
			nnz++;
		}
	}	
	
	if(CPXcallbackaddusercuts(cut_pars.context, 1, nnz, &rhs, &sense, &izero, index, value, &purgeable, &local)) print_state(Error, "CPXcallbackaddusercuts() error");

	free(index);
	free(value);
	return 0; 
}


int add_SEC_flt(CPXCALLBACKCONTEXTptr context,TSPinst inst){

	int nodeid = -1; 
	CPXcallbackgetinfoint(context,CPXCALLBACKINFO_NODEUID,&nodeid);
	if(nodeid%10) return 0;

	int ncols = inst.nnodes*(inst.nnodes-1)/2;
	double* xstar = (double*) malloc(ncols * sizeof(double));
    double* xstar2 = (double*) calloc(ncols, sizeof(double));  
	double objval = CPX_INFBOUND; 

	if (CPXcallbackgetrelaxationpoint(context, xstar, 0, ncols-1, &objval)) print_state(Error, "CPXcallbackgetcandidatepoint error");

	int* elist = (int*) malloc(2*ncols*sizeof(int));  
	int ncomp = -1;
	int* compscount = (int*) NULL;
	int* comps = (int*) NULL;


    int k=0;
    int n = 0;
	for(int i = 0; i<inst.nnodes; i++){
		for(int j=i+1;j<inst.nnodes;j++){
            int f = coords_to_index(inst.nnodes, i, j);
            if(xstar[f] <= EPSILON) continue; 
			elist[k]=i;
			elist[++k]=j;
			k++;
            xstar2[n++]  = xstar[f];
		}
	}

	CCcut_connect_components(inst.nnodes,n,elist,xstar2,&ncomp,&compscount,&comps);

	cut_par user_handle= {context,inst.nnodes};
	if(ncomp ==1) {
		#if VERBOSE > 1
			printf("\e[1mBRANCH & CUT\e[m \t%4d \e[3mFLOW cut\e[m found\n",ncomp);
		#endif

		CCcut_violated_cuts(inst.nnodes,ncols,elist,xstar2,1.9,add_cut_CPLEX,(void*) &user_handle);
	}
	else {
		#if VERBOSE > 1
			printf("\e[1mBRANCH & CUT\e[m \t%4d \e[3mRELAXATION cuts\e[m found\n",ncomp);
		#endif

		int start = 0;
		for(int k=0;k<ncomp;k++){
			int* node_indeces = (int*) malloc(compscount[k]*sizeof(int));

			for(int i=0;i<compscount[k];i++) 
				node_indeces[i]=comps[i+start];
			start += compscount[k];

			add_cut_CPLEX(0.0,compscount[k],node_indeces,(void*) &user_handle);
			free(node_indeces);
		}

		
	}

	free(xstar);
    free(xstar2);
	free(elist);
	free(compscount);
	free(comps);
	return 0;
}


/// @brief Callback function to add sec to cut pool
/// @param context callback context pointer
/// @param contextid context id
/// @param userhandle data passed to callback
int CPXPUBLIC mount_CUT(CPXCALLBACKCONTEXTptr context, CPXLONG contextid, void* userhandle) { 
	TSPinst inst = * (TSPinst*) userhandle;
	switch(contextid){
		case CPX_CALLBACKCONTEXT_CANDIDATE:  return add_SEC_int(context,inst);
		case CPX_CALLBACKCONTEXT_RELAXATION: return add_SEC_flt(context,inst); 
		default: print_state(Error, "contextid unknownn in add_SEC_callback"); return 1;
	} 
}


extern void add_warm_start(CPXENVptr CPX_env, CPXLPptr CPX_lp, TSPinst* inst, TSPenv* env, char* method) {
	char* curr_meth = malloc(strlen(env->method));
	memcpy(curr_meth, env->method, strlen(env->method));
	env->method = method;

	TSPsolve(inst, env);

	env->method = curr_meth;
	CPLEX_post_heur(CPX_env, CPX_lp, inst->solution, inst->nnodes);

	#if VERBOSE > 0
		print_state(Info, "passing an heuristic solution to CPLEX...\n");
	#endif
}