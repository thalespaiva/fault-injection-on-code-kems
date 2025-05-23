#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utils.h"

void init_set(Set *set, int limit) {
    set->n = 0;
    set->limit = limit + 1;
    set->indexes = malloc(set->limit*sizeof(*(set->indexes)));
    memset(set->indexes, NOT_IN_SET, set->limit*sizeof(*(set->indexes)));
    set->entries = calloc(set->limit, sizeof(*(set->entries)));
}

void reinit_set(Set *set) {
    int limit = set->limit;
    free_set(set);
    init_set(set, limit);
}

int int_cmp(const void * a, const void * b) {
   return ( *(const int*)a - *(const int*)b );
}

void sort_set(Set *support) {
    qsort(support->entries, support->n, sizeof(*support->entries), int_cmp);
    for (int i = 0; i < support->n; i++)
        support->indexes[support->entries[i]] = i;
}

void free_set(Set *set) {
    set->n = 0;
    set->limit = 0;
    free(set->indexes);
    free(set->entries);
}

int is_in_set(Set *set, int element) {
    return (set->indexes[element] >= 0);
}

void remove_from_set(Set *set, int element) {
    if (is_in_set(set, element)) {
        int index = set->indexes[element];

        int last_element = set->entries[set->n - 1];
        set->indexes[last_element] = index;
        set->entries[index] = last_element;

        set->n--;
        set->indexes[element] = NOT_IN_SET;
    }
}

void copy_set(Set *to, Set *from) {
    init_set(to, from->limit - 1);
    for (int i = 0; i < from->n; i++)
        add_to_set(to, from->entries[i]);
}

int add_to_set(Set *set, int value) {
    if (value > set->limit) {
        printf("v %d l %d\n", value, set->limit);
        fflush(stdout);
    }
    assert (value <= set->limit);
    assert (value >= 0);

    if (is_in_set(set, value))
        return 0;
    set->entries[set->n] = value;
    set->indexes[value] = set->n;
    set->n++;
    return 1;
}

void pop_from_set(Set *set) {
    int element = set->entries[set->n - 1];
    set->indexes[element] = NOT_IN_SET;
    set->n--;
}


void set_print(Set *set) {
    for (int i = 0; i < set->n; ++i) {
        printf("%d ", set->entries[i]);
    }
    printf("\n");
}

/* Floyd's algorithm */
// for J := N-M + 1 to N do
//     T := RandInt(1, J)
//     if T is not in S then
//         insert T in S
//     else
//         insert J in S

void select_from_set(Set *selection, Set *set, int k) {

    if (k >= set->n) {
        for (int i = 0; i < set->n; i++)
            add_to_set(selection, set->entries[i]);
        return;
    }

    Set aux;
    init_set(&aux, set->limit);

    for (int j = set->n - k; j < set->n; j++) {
        int t = rand() % j;
        if (!is_in_set(&aux, t))
            add_to_set(&aux, t);
        else
            add_to_set(&aux, j);
    }

    for (int i = 0; i < k; i++)
        add_to_set(selection, set->entries[aux.entries[i]]);


    free_set(&aux);
}

void swap(int *a, int *b) {
    int tmp;

    tmp = *a;
    *a = *b;
    *b = tmp;
}

void matrix_print(Matrix *matrix) {
    for (int i = 0; i < matrix->nrows; ++i) {
        for (int j = 0; j < matrix->ncols; ++j)
            printf("%d", matrix->entries[i][j]);
        printf("\n");
    }
}

void init_matrix(Matrix *matrix, int nrows, int ncols) {
    matrix->nrows = nrows;
    matrix->ncols = ncols;

    matrix->entries = malloc(nrows*sizeof(*matrix->entries));
    for (int i = 0; i < nrows; ++i) {
        matrix->entries[i] = malloc(ncols*sizeof(*matrix->entries[i]));
        memset(matrix->entries[i], 0, ncols*sizeof(*matrix->entries[i]));
    }
}

void free_matrix(Matrix *matrix) {
    for (int i = 0; i < matrix->nrows; ++i)
        free(matrix->entries[i]);
    free(matrix->entries);
    matrix->nrows = 0;
    matrix->ncols = 0;
}



void print_sparse(SparseArray *array) {
    int i;

    for (i = 0; i < array->n_positions; i++)
        printf("%d ", array->positions[i]);
    printf("\n");
}

void file_print_sparse(FILE *file, SparseArray *array) {
    int i;

    for (i = 0; i < array->n_positions; i++)
        fprintf(file, "%d ", array->positions[i]);
    fprintf(file, "\n");
}


void bin_array_init_random(Bit array[], int n, int weight) {
    int i, pos, tmp;

    memset(array, 0, n*sizeof(*array));
    for (i = 0; i < weight; i++)
        array[i] = 1;

    /* Fisher-Yates */
    for (i = n - 1; i >= 1; i--) {
        pos = rand() % (i + 1);
        tmp = array[pos];
        array[pos] = array[i];
        array[i] = tmp;
    }
}



int array_get_max(int array[], int n) {
    int i, max;

    max = array[0];
    for (i = 1; i < n; i++)
        if (max < array[i])
            max = array[i];

    return max;
}

void array_print(int array[], int n) {
    int i;

    for (i = 0; i < n; i++)
        printf("%d", array[i]);
    printf("\n");

}

void bit_array_print(Bit array[], int n) {
    int i;

    for (i = 0; i < n; i++)
        printf("%d", array[i]);
    printf("\n");

}

void file_array_print(FILE *file, int array[], int n) {
    int i;

    for (i = 0; i < n; i++)
        fprintf(file, "%d ", array[i]);
    fprintf(file, "\n");

}

void bin_array_print(int array[], int n) {
    int i, j;
    int tmp;
    int cols;

    cols = 0;
    for (i = 0; i < n; i += 8) {
        tmp = 0;
        for (j = i; j < i + 8 && j < n; j++) {
            tmp += array[j] << ((i + 8 - 1) - j);
        }
        printf("%02x ", tmp);
        cols += 2 + 1;
        if (cols + 3 > 80) {
            cols = 0;
            printf("\n");
        }
    }
    printf("\n");

}
