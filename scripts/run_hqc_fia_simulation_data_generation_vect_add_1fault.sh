#!/usr/bin/env bash
set -euo pipefail

mkdir -p results/hqc/1fault_vect_add/

cd hqc
rm -rf tmp_build_hqc_avx256

echo "[*] Generating data for HQC Level 1 considering 1 fault on vect_add"
echo "[*] Simultion will be done using AVX2 HQC implementation"

cmake -B tmp_build_hqc_avx256 -DCMAKE_BUILD_TYPE=Release -DHQC_ARCH=x86_64 -DHQC_X86_IMPL=avx256 &&
make -C tmp_build_hqc_avx256

cd ..

max_jobs=10
total_seeds=10
trials=50000000

for seed in $(seq 1 "$total_seeds"); do
  ./hqc/tmp_build_hqc_avx256/fia_simulation/fia_simulation_hqc_1 "$seed" "$trials" \
    > "results/hqc/1fault_vect_add/hqc1_seed${seed}_50M_vect_add_1fault.csv" &

  while [ "$(jobs -rp | wc -l)" -ge "$max_jobs" ]; do
    wait -n
  done
done

wait
echo "All jobs finished."

rm -rf tmp_build_hqc_avx256
