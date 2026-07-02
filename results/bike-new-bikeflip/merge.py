import pandas as pd

N_PARTS = 10

dfs = [pd.read_csv('../../results/bike-new-bikeflip/exp1_raw/bike_new_bikeflip1_seed1_part%i.csv.xz' % i) for i in range(1, N_PARTS + 1)]

dfs = [df[df.n_challenges == 2000000] for df in dfs]
dfs = [df.drop(columns=['spec_e0_estimator', 'spec_e1_estimator']) for df in dfs]
sum_cols = [
    "n_challenges",
    "spec_e0_failure",
    "spec_e0_count",
    "spec_e1_failure",
    "spec_e1_count",
]

keep_cols = [
    "distance",
    "spec_h0",
    "spec_h1",
]


# consolidated = dfs[0][keep_cols].copy()
# consolidated[sum_cols] = dfs[0][sum_cols]

consolidated = pd.DataFrame()

for i in range(1, N_PARTS + 1):
    df_sum = dfs[0][keep_cols].copy()
    df_sum[sum_cols] = sum(df[sum_cols] for df in dfs[:i])

    df_sum['spec_e0_estimator'] = df_sum.spec_e0_failure/df_sum.spec_e0_count
    df_sum['spec_e1_estimator'] = df_sum.spec_e1_failure/df_sum.spec_e1_count

    # df_sum.to_csv('../../results/bike-new-bikeflip/bike_new_bikeflip1_seed1_merged.csv', index=False)

    consolidated = pd.concat([consolidated, df_sum])
    # df = consolidated

df = consolidated
df.to_csv('../../results/bike-new-bikeflip/bike_new_bikeflip1_seed1.csv', index=False)

# sns.histplot(df[df.n_challenges == max(df.n_challenges)], x='spec_e0_estimator', hue='spec_h0')


# run_key_recovery_against_real_dataset_bike('../../results/bike-new-bikeflip/bike_new_bikeflip1_seed1_merged.csv', 71, 2000000, 1500, 3500)
