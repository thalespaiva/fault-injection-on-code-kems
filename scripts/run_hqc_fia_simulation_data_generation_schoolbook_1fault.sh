#!/usr/bin/env bash
set -euo pipefail

mkdir -p results/hqc/1fault_schoolbook_mul

max_jobs=5
total_seeds=50

for seed in $(seq 1 "$total_seeds"); do
  ./build/hqc/fia_simulation/fia_simulation_cbz_hqc_1 "$seed" \
    > "results/hqc/1fault_schoolbook_mul/hqc1_seed${seed}.csv" &

  while [ "$(jobs -rp | wc -l)" -ge "$max_jobs" ]; do
    wait -n
  done
done

wait
echo "All jobs finished."
