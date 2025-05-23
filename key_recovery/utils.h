#ifndef UTILS_H
#define UTILS_H

#define MAX_SPARSE_POSITIONS 100000


#define MOD(a,b) ((((a)%(b))+(b))%(b))

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#include "bittypes.h"

typedef struct _sparse_array_t {
    int positions[MAX_SPARSE_POSITIONS];
    int n_positions;
} SparseArray;

typedef struct _set {
    int n;
    int limit;
    int *entries;
    int *indexes;
} Set;

#define NOT_IN_SET -1

typedef struct _matrix {
    int **entries;
    int nrows;
    int ncols;
} Matrix;

void swap(int *a, int *b);
void init_set(Set *set, int limit);
int add_to_set(Set *set, int value);
void set_print(Set *set);
void matrix_print(Matrix *matrix);
void init_matrix(Matrix *matrix, int nrows, int ncols);
void free_matrix(Matrix *matrix);
void free_set(Set *set);
void remove_from_set(Set *set, int element);
void pop_from_set(Set *set);
int is_in_set(Set *set, int element);
void copy_set(Set *to, Set *from);
void sort_set(Set *support);
void reinit_set(Set *set);

/* Fisher-Yates */
void select_from_set(Set *selection, Set *set, int k);

int array_get_max(int array[], int n);
void array_print(int array[], int n);
void bit_array_print(Bit array[], int n);
void file_array_print(FILE *file, int array[], int n);
void bin_array_print(int array[], int n);
void print_sparse(SparseArray *array);
void file_print_sparse(FILE *file, SparseArray *array);
void bin_array_init_random(Bit array[], int n, int weight);

#endif
