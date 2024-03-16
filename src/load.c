#include "../include/load.h"

void parse_cli(int argc, char **argv,cli_info* cli_data){
    cli_data->nnodes=0;
    cli_data->random_seed=0;
    cli_data->thread = 0;
    cli_data->time_limit = MAX_TIME;
    strcpy(cli_data->method,"");
    strcpy(cli_data->file_name,"");

    for (int i = 1; i < argc; i++) { 
        if (!strcmp(argv[i],"-tl") ||!strcmp(argv[i],"-max_time")) cli_data->time_limit = abs(atoi(argv[++i])+1);
        if (!strcmp(argv[i],"-in")  || !strcmp(argv[i],"-f")  || !strcmp(argv[i],"-file"))  strcpy(cli_data->file_name,argv[++i]);
        if (!strcmp(argv[i],"-n") || !strcmp(argv[i],"-n_nodes")) cli_data->nnodes = abs(atoi(argv[++i]));
        if (!strcmp(argv[i],"-seed") || !strcmp(argv[i],"-rnd_seed") || !strcmp(argv[i],"-s")) cli_data->random_seed = abs(atoi(argv[++i])); 
        if (!strcmp(argv[i],"-algo") || !strcmp(argv[i],"-method") || !strcmp(argv[i],"-alg")) strcpy(cli_data->method,argv[++i]);
        if (!strcmp(argv[i],"-thread") || !strcmp(argv[i],"-th") || !strcmp(argv[i],"-core")) cli_data->thread = 1;
        if (!strcmp(argv[i],"-help")  || !strcmp(argv[i],"--help") || !strcmp(argv[i],"-h"))   {help_info(); exit(0);}  
    } 

    if(cli_data->nnodes && cli_data->random_seed) strcpy(cli_data->file_name,"RND");
}