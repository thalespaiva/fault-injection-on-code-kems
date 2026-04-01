cd bike
rm -r tmp_build_bike

echo "[*] Generating data for BIKE Level 1..."
cmake -DLEVEL=1 -B tmp_build_bike
make -C tmp_build_bike
./tmp_build_bike/gen-hw-s-plot-data 1 1000 | tee ../data/data_bike_fia_effect_on_threshold.csv
rm -r tmp_build_bike
echo "[+] Done"

echo "[*] Generating data for BIKE Level 3..."
cmake -DLEVEL=3 -B tmp_build_bike
make -C tmp_build_bike
./tmp_build_bike/gen-hw-s-plot-data 1 1000 | tail -n +2 | tee -a ../data/data_bike_fia_effect_on_threshold.csv
rm -r tmp_build_bike
echo "[+] Done"


echo "[*] Generating data for BIKE Level 5..."
cmake -DLEVEL=5 -B tmp_build_bike
make -C tmp_build_bike
./tmp_build_bike/gen-hw-s-plot-data 1 1000 | tail -n +2 | tee -a ../data/data_bike_fia_effect_on_threshold.csv
rm -r tmp_build_bike
echo "[+] Done"
