#!/usr/bin/env bash
set -euo pipefail

mkdir -p results/bike

max_jobs=5
total_seeds=10
trials=5000000

for seed in $(seq 1 "$total_seeds"); do
  ./build/bike/bike-fia-simulation "$seed" "$trials" \
    > "results/bike/bike1_seed${seed}_FIAerror0.004_5M.csv" &

  while [ "$(jobs -rp | wc -l)" -ge "$max_jobs" ]; do
    wait -n
  done
done

wait
echo "All jobs finished."
