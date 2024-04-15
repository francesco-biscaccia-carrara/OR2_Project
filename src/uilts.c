#include "../include/utils.h"

/// @brief used to print logical errors whenever they occur
/// @param error_message text to print
void print_error(const char *error_message){
    printf("\n\x1b[31mERROR: %s\x1b[0m\n", error_message); 
    fflush(NULL); 
    exit(1);
} 


#define INDEX(n,i,j) (i * (n - 0.5*i - 1.5) + j -1)
/// @brief transform 2d coordinate for a triangular matrix in 1d array
/// @param n number of rows
/// @param i row index
/// @param j column index
/// @return index where the desired value is stored into a 1d array
int coords_to_index(const unsigned int n,const int i,const int j){
    if (i == j) print_error("i == j");
    return i<j ? INDEX(n,i,j) : INDEX(n,j,i);
}


/// @brief get current execution time
/// @return current execution time
double get_time(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    return ((double)tv.tv_sec)+((double)tv.tv_usec/1e+6);
}


/// @brief get elapsed time from certain intial time
/// @param initial_time starting time
/// @return time passed from initial_time
double time_elapsed(const double initial_time) {
    
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((double)tv.tv_sec)+((double)tv.tv_usec/1e+6) - initial_time;
}


/// @brief reverse an array of integers passed as a parameter from one index to another. 
/// @param array target array
/// @param from intial index where reverse start
/// @param to terminal index of reverse operation
void reverse(int* array, unsigned int from, unsigned int to){
    while(from<to){
        int tmp = array[from];
        array[from]=array[to];
        array[to]=tmp;
        from++;to--;
    }
}


/// @brief get number of unique integer element inside an array
/// @param inst array of integer 
/// @param size size of array
/// @return number of unque values
int arrunique(const int* inst, const unsigned int size) {
    if(size <= 0) print_error("input not valid");
    int elem[size];
    elem[0] = inst[0];
    int out = 1;
    char seen = 0;

    for(int i = 0; i < size; i++) {
        for(int j = 0; j < out; j++) {
            if((seen = (elem[j] == inst[i])) != 0) break;
        }

        if(!seen) elem[out++] = inst[i];
        seen = 0;
    }
    return out;
}


/// @brief check if a string is inside an array
/// @param target string to check 
/// @param array array of strings
/// @param array_size size of array
/// @return 0 if target is not in the array, otherwise 1 
char strnin(const char* target,char** array, const size_t array_size){

    size_t wlen = strlen(target);

    for(int i = 0; i < array_size; i++) {
        size_t slen = strlen(array[i]); 
        if(wlen != slen) continue;
        if(!strncmp(target, array[i], slen)) return 1;
    }    
    return 0;
}


/// @brief convert a solution obtained by cplex into solution in heuristic format 
/// @param hsol heuristic solution format
/// @param csol CPLEX solution format
/// @param array_size number of nodes (length of csol)
/// @return pointer where solution is stored
int* cth_convert(int* hsol, int* csol, const unsigned int array_size) {
    int j = csol[0];
	for(int i = 0; i<array_size; i++) {
		hsol[i] = j;
		j = csol[j];
	}

    return hsol;
}


/// @brief write an array into csv file
/// @param dest destination file 
/// @param content array to write inside csv file
/// @param size length of content
void format_csv_line(FILE* dest, const double* content, const unsigned int size) {
    fprintf(dest, "%f", content[0]);
    for(int i = 1; i < size; i++) {
        fprintf(dest, ",%f",content[i]);
    }
    fprintf(dest, "\n");
}