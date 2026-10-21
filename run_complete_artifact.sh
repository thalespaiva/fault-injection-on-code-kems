#!/bin/bash

echo "Running the complete artifact for the paper"
echo "Generating the corresponding data"
echo "1. HQC's polynomial multiplication"
./scripts/run_hqc_fia_simulation_data_generation_schoolbook_1fault.sh

echo "2. HQC's vector addition"
./scripts/run_hqc_fia_simulation_data_generation_vect_add_1fault.sh

echo "Finished HQC's data generation. Starting the experiment for BIKE"
echo "3. BIKE's BGF threshold"
./scripts/run_bike_fia_simulation_data_generation.sh

echo "4. BIKE's BIKE-flip"
./scripts/run_bike_bikeflip_fia_simulation_data_generation.sh
echo "Merging BIKE-flip dataset"
python3 merge.py

echo "Finished the data generation phase"
echo "Running the attacks"
python3 key_recovery/spec_rec.py

# This last step for key recovery uses sage to estimate attack cost.
# sage --version: 10.9
# python --version: Python 3.14.7
python key_recovery/hqc_fia_schoolbook_key_recovery.sage
