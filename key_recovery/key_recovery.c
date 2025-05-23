#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include <time.h>



#include "utils.h"


typedef struct _secret_vector {
    int r;
    int *v;
    int w;
    int *sp;
} SecretVector;

typedef struct _common_params {
    Set *possible_positions;
    Set *outside_distances;
    int population_size;
    int s;
    int weight;
    int r;
    SecretVector *sv;
} Params;


int int_cmp(const void * a, const void * b);

void init_random_secret_vector(SecretVector *sv, int r, int w) {

    sv->v = calloc(r, sizeof(*(sv->v)));
    sv->sp = calloc(w, sizeof(*(sv->v)));


    int count = 0;
    do {
        int idx = rand() % r;
        if (!sv->v[idx]) {
            sv->v[idx] = 1;
            sv->sp[count] = idx;
            count++;
        }
    } while (count < w);

    sv->r = r;
    sv->w = w;

    qsort(sv->sp, sv->w, sizeof(int), int_cmp);

}

void secret_vector_free(SecretVector *sv) {
    free(sv->v);
    free(sv->sp);
}

int check_if_is_key(SecretVector *sv, Set *candidate_key);

void free_params(Params *params) {
    params->population_size = -1;
    params->s = -1;
    params->weight = -1;
    params->r = -1;
    params->sv = NULL;
    free_set(params->possible_positions);
    free(params->possible_positions);
}

int get_distance(int pos1, int pos2, int length) {
    int dist1, dist2;

    dist1 = abs(pos2 - pos1);
    dist2 = length - dist1;
    if (dist1 <= dist2)
        return dist1;
    return dist2;
}


void init_params(Params *params, Set *outside_distances,
                 int population_size, int s, int r, int weight, SecretVector *sv) {

    params->outside_distances = outside_distances;
    params->population_size = population_size;
    params->s = s;
    params->r = r;
    params->weight = weight;
    params->sv = sv;

    Set *possible_positions = malloc(sizeof(*possible_positions));
    init_set(possible_positions, r);

    for (int i = 0; i < r; i++) {
        if (i == s || i == 0)
            continue;

        int d0 = get_distance(0, i, r);
        int d1 = get_distance(s, i, r);

        if (!is_in_set(outside_distances, d0) && !is_in_set(outside_distances, d1))
            add_to_set(possible_positions, i);
    }

    params->possible_positions = possible_positions;

}


void get_spectrum(SecretVector *sv, Set *spectrum) {

    init_set(spectrum, sv->r/2 + 1);

    printf("Targeting key:\n");
    for (int i = 0; i < sv->w; i++) {
        int ipos = sv->sp[i];

        printf("%d ", ipos);

        for (int j = i + 1; j < sv->w; j++) {
            int jpos = sv->sp[j];
            add_to_set(spectrum, get_distance(ipos, jpos, sv->r));
        }
    }
    sort_set(spectrum);
}

void select_from_out_spectrum(Set *selection, int k, Set *spectrum, int r) {

    Set out_spectrum;
    init_set(&out_spectrum, r/2 + 1);

    for (int i = 1; i <= r/2; i++) {
        if (!is_in_set(spectrum, i))
            add_to_set(&out_spectrum, i);
    }

    init_set(selection, r/2 + 1);
    select_from_set(selection, &out_spectrum, k);

    free_set(&out_spectrum);
}

int build_support(Params *params, Set *support) {

    Set composed_possible;
    copy_set(&composed_possible, params->possible_positions);

    int *impossible = malloc(params->r*sizeof(*impossible));

    int missing = params->weight - 2 - support->n;
    while (missing > 0 && composed_possible.n >= missing) {

        int n_impossible = 0;

        int rand_val;
        #pragma omp critical
        rand_val = rand();

        int rand_next = composed_possible.entries[rand_val % composed_possible.n];
        add_to_set(support, rand_next);
        remove_from_set(&composed_possible, rand_next);

        for (int i = 0; i < composed_possible.n; i++) {
            int pos_i = composed_possible.entries[i];
            int flag_is_possible = 1;
            for (int j = 0; j < support->n; j++) {
                int pos_j = support->entries[j];

                int dist = get_distance(pos_i, pos_j, params->r);
                if (is_in_set(params->outside_distances, dist)) {
                    flag_is_possible = 0;
                    break;
                }
            }
            if (!flag_is_possible) {
                impossible[n_impossible++] = pos_i;
            }
        }

        for (int i = 0; i < n_impossible; i++)
            remove_from_set(&composed_possible, impossible[i]);
        missing = params->weight - 2 - support->n;
    }

    free(impossible);
    free_set(&composed_possible);
    return (support->n);
}


void transform_support_in_complement(Params *params, Set *support) {

    Set *new_support = malloc(sizeof(*new_support));
    init_set(new_support, support->limit);

    sort_set(support);
    int min_dist = get_distance(support->entries[0], support->entries[1], params->r);
    int i0 = support->entries[0];
    for (int i = 1; i < support->n; i++) {
        int dist = get_distance(support->entries[i],
                                support->entries[MOD(i + 1, support->n)], params->r);
        if (dist < min_dist) {
            i0 = support->entries[i];
        }
    }


    add_to_set(new_support, support->entries[i0]);
    add_to_set(new_support, MOD(support->entries[i0] + min_dist, params->r));

    for (int i = 2; i < support->n; i++) {
        int pw = support->entries[MOD(i0 - i + 1, support->n)];
        add_to_set(new_support, MOD(params->r - pw + new_support->entries[1], params->r));
    }

    free_set(support);
    copy_set(support, new_support);
    free_set(new_support);
    free(new_support);
}


long long unsigned NumberOfPaths;

int probabilistic_attack(Set *key, Params *params) {

    int key_found_flag = 0;

    sort_set(params->possible_positions);


    NumberOfPaths = 0;

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < params->population_size; i++) {
        // printf("d\n");
        if (key_found_flag == 1)
            continue;

        Set tmp_support;
        init_set(&tmp_support, params->r);

        if ((i + 1) % 1000000 == 0) {
            printf("\r%d", i + 1);
            fflush(stdout);
        }

        if (build_support(params, &tmp_support) >= params->weight - 2) {
            int current_number_of_paths = NumberOfPaths;
            add_to_set(&tmp_support, 0);
            add_to_set(&tmp_support, params->s);
            sort_set(&tmp_support);

            #pragma omp critical
            {
                if (key_found_flag == 0) {
                    int check = check_if_is_key(params->sv, &tmp_support);
                    printf("[+] POSSIBLE KEY FOUND : \n");

                    set_print(&tmp_support);
                    if (!check) {
                        transform_support_in_complement(params, &tmp_support);
                        printf("[.] Transforming\n");
                        check = check_if_is_key(params->sv, &tmp_support);
                    }

                    if (check) {
                        printf("[+] IS KEY : YES\n");
                        printf("[+] NITERATIONS : %d\n", current_number_of_paths);
                        key_found_flag = 1;
                        copy_set(key, &tmp_support);
                    }
                    else {
                        printf("[-] IS KEY : NO \n");
                    }
                }
            }
        }
        #pragma omp critical
        NumberOfPaths++;
        free_set(&tmp_support);
    }
    return key_found_flag;
}

#define MAX_R 100000

int check_if_is_key(SecretVector *sv, Set *candidate_key) {

    int times[MAX_R] = {0};
    int last[MAX_R] = {0};

    for (int i = 0; i < sv->w - 1; i++) {
        times[sv->sp[i + 1] - sv->sp[i]] += 1;
        last[sv->sp[i + 1] - sv->sp[i]] = sv->sp[i];
    }

    int unique = -1;
    for (int i = 0; i < sv->r; i++) {
        if (times[i] == 1) {
            unique = i;
            break;
        }
    }

    int shifted_sv[MAX_R] = {0};
    for (int i = 0; i < sv->w; i++) {
        int sind = MOD(sv->sp[i] - last[unique], sv->r);
        shifted_sv[sind] = 1;
    }

    sort_set(candidate_key);
    int cand_shift_value = -1;
    for (int i = 0; i < sv->w - 1; i++) {
        if (candidate_key->entries[i + 1] - candidate_key->entries[i] == unique) {
            cand_shift_value = candidate_key->entries[i];
            break;
        }
    }

    for (int i = 0; i < sv->w - 1; i++) {
        int shifted_cand_tmp = MOD(candidate_key->entries[i] - cand_shift_value, sv->r);
        if (!shifted_sv[shifted_cand_tmp])
            return 0;
    }
    return 1;
}


int main(int argc, char *argv[]) {

    if (argc < 8 || argc > 10) {
        printf("Usage: <r> <w> <ntests> <n_out_spectrum> <n_tries> <seed> <nthreads>");
        printf("[out_spec_step, out_spec_lim]\n");
        exit(1);
    }

    int r = atoi(argv[1]);
    int w = atoi(argv[2]);
    int ntests = atoi(argv[3]);
    int total_n_out_spectrum = atoi(argv[4]);
    int n_individuals = atoi(argv[5]);
    int seed = atoi(argv[6]);
    int nthreads = atoi(argv[7]);

    if (nthreads > 0)
        omp_set_num_threads(nthreads);

    int out_spec_step = total_n_out_spectrum;
    int out_spec_lim = total_n_out_spectrum;
    if (argc > 8) {
        out_spec_step = atoi(argv[8]);
        out_spec_lim = atoi(argv[9]);
    }


    srand(seed);


    for (int n_out_spectrum = total_n_out_spectrum; n_out_spectrum >= out_spec_lim;
                                                    n_out_spectrum -= out_spec_step) {

        printf("=============== New battery n_out_spec = %4d ============\n\n", n_out_spectrum);


        for (int itest = 0; itest < ntests; itest++) {

            printf("------------ Starting Test %4d ---------------\n", itest + 1);

            SecretVector sv;
            init_random_secret_vector(&sv, r, w);


            Set spectrum;

            get_spectrum(&sv, &spectrum);
            int s = spectrum.entries[rand() % spectrum.n];


            Set out_spectrum;
            select_from_out_spectrum(&out_spectrum, n_out_spectrum, &spectrum, r);

            Set considered_out;
            init_set(&considered_out, r);

            double frac = 0;
            select_from_set(&considered_out, &spectrum, (int) (frac*spectrum.n));
            remove_from_set(&considered_out, s);

            for (int i = 0; i < out_spectrum.n; i++)
                add_to_set(&considered_out, out_spectrum.entries[i]);


            Params params;
            init_params(&params, &considered_out, n_individuals, s, r, w, &sv);

            Set key;

            int success_flag;

            printf("[.] # possible positions = %d\n", params.possible_positions->n);
            fflush(stdout);

            struct timespec start, end, real_start, real_end;
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
            clock_gettime(CLOCK_REALTIME, &real_start);

            success_flag = probabilistic_attack(&key, &params);
            if (success_flag)
                free_set(&key);
            else
                printf("[-] Failed\n");
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
            clock_gettime(CLOCK_REALTIME, &real_end);

            double delta_s = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1.0e9;
            double delta_s_wall = (real_end.tv_sec - real_start.tv_sec) + (real_end.tv_nsec - real_start.tv_nsec)/1.0e9;
            printf("[+] wall time = %.16lf s\n", delta_s_wall);
            printf("[+] cpu time = %.16lf s\n", delta_s);

            free_set(&considered_out);
            free_set(&spectrum);
            free_set(&out_spectrum);
            free_params(&params);
            secret_vector_free(&sv);

            if (!success_flag) {
                printf("[-] Stopping Attack. Try a larger number of constructions\n");
            }
            printf("-----------------------------------------------\n\n");
        }
        printf("==========================================================\n");
    }
}