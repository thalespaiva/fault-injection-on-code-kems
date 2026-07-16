from sage.all import *
import pandas as pd
from ast import literal_eval

import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns


def parse_fia_simulation_csv(datafile):
    list_converter = lambda x: map(int, )

    df_xyhs = pd.read_csv(datafile, converters={
        "x": literal_eval,
        "y": literal_eval,
        "h": literal_eval,
        "s": literal_eval,
    }, nrows=1)

    x = df_xyhs.x[0]
    y = df_xyhs.y[0]
    h = df_xyhs.h[0]
    s = df_xyhs.s[0]

    df = pd.read_csv(datafile, skiprows=2)

    return (x, y), (h, s), df

def extract_set_z_of_known_null_entries(df):
    f = list(df.failure_rate)
    n = len(df)

    zeros = set(range(n))
    for block in range(n//64 + 1):
        zeros.discard(block * 64)
        for i in range(1, 64):
            idx = block * 64 + i
            if idx >= n:
                break
            if f[idx] == 0 and f[idx - 1] == 1:
                for l in range(4):
                    zeros.discard(block * 64 + i + l)

    return zeros

def extract_set_z_of_known_null_entries_optimized(df):
    f = list(df.failure_rate)
    n = len(df)

    # Discard n//64 entries of f (less decapsulations) by
    # conservatively fixing them as 1
    for i in range(n//64 + 1):
        f[64*i] = 1

    zeros = set(range(n))
    for block in range(n//64 + 1):
        zeros.discard(block * 64)
        for i in range(1, 64):
            idx = block * 64 + i
            if idx >= n:
                break
            if f[idx] == 0 and f[idx - 1] == 1:
                for l in range(4):
                    zeros.discard(block * 64 + i + l)

    return zeros


def test_if_key_recovery_is_possible(datafile, optimized=False):
    (x, y), (h, s), df = parse_fia_simulation_csv(datafile)

    n = len(df)
    w = len(x)

    # print(y)
    # y_candidate_support = extract_possible_support(df)
    if optimized:
        y_candidate_support = set(range(n)) - extract_set_z_of_known_null_entries_optimized(df)
    else:
        y_candidate_support = set(range(n)) - extract_set_z_of_known_null_entries(df)


    # print(f'{len(y_candidate_support)=}')
    # print(f'{len(set(y) - set(y_candidate_support))=}')
    if (len(set(y) - set(y_candidate_support))):
        print('[-] Oh no, key recovery is not possible!')
        print('set(y) - set(y_candidate_support) = ', set(y) - set(y_candidate_support))
        return None

    n_zeros_needed = len(y_candidate_support)
    complexity = 1 / (binomial(n - w, n_zeros_needed)/binomial(n, n_zeros_needed))
    # print(f'[* {datafile}] Attack cost = ', numerical_approx(complexity))

    # return [numerical_approx(complexity), n - len(y_candidate_support)]
    return numerical_approx(complexity)



if __name__ == '__main__':

    costs_not_optimized = []
    costs_optimized = []
    for i in range(1, 51):
        cost = test_if_key_recovery_is_possible(f'results/hqc/1fault_schoolbook_mul/hqc1_seed{i}.csv')
        cost_opt = test_if_key_recovery_is_possible(
            f'results/hqc/1fault_schoolbook_mul/hqc1_seed{i}.csv', optimized=True)
        costs_not_optimized.append(cost)
        costs_optimized.append(cost_opt)

    # print(costs)

    n = 17669
    print(f'[+] Avg cost (not optimized): {n} decaps and {np.mean(costs_not_optimized)} guesses')
    print(f'[+] Avg cost (optimized): {n - n//64} decaps and {np.mean(costs_optimized)} guesses')

