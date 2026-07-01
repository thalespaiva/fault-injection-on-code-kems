#!/usr/bin/env bash
set -euo pipefail

mkdir -p results/bike-new-bikeflip
make -C bike-new-bikeflip

max_jobs=10
total_seeds=10
trials=2000000

for encaps_seed in $(seq 1 "$total_seeds"); do
  ./bike-new-bikeflip/bike-flip-fia-simulation "1" "$trials" "$encaps_seed" \
    > "results/bike-new-bikeflip/bike_new_bikeflip1_seed1_part${encaps_seed}.csv" &

  while [ "$(jobs -rp | wc -l)" -ge "$max_jobs" ]; do
    wait -n
  done
done

wait
echo "All jobs finished."
