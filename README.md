# Exploring fault injection attacks on HQC and BIKE beyond the Fujisaki-Okamoto verification

**Abstract**. This repository contains the code and data associated with our
paper "Exploring fault injection attacks on HQC and BIKE beyond the
Fujisaki-Okamoto verification".
Our implementation includes 2 main components: (i) code to fully reproduce the key
recovery attacks on HQC and BIKE, and (ii) the dataset obtained from our
experiments in software. Additionally, we also include the code to reproduce
the figures in the paper, and the analysis of the datasets.

## Important notices

**1. External code.** In this repository, we use external codes provided by both
HQC's and BIKE's Team to simulate the fault injection attack at software level.
The three repositories are listed as follows.

* [HQC's reference implementation](https://gitlab.com/pqc-hqc/hqc/)
* [BIKE's reference implementation with Bit Flip decoder](https://bikesuite.org/files/v5.2/Reference_Implementation.2024.10.10.1.zip)
* [BIKE's reference implementation with BGF decoder](https://github.com/awslabs/bike-kem)

**2. On the exact reproduction of data.** To exactly reproduce the results of
the software simulation, experiments usually take a few hours depending on
the considered KEM and the location of the fault. The execution of the
experiments is done according to the bash scripts located in the ```scripts/```
directory.

**3. Running scripts.** The complete execution of the running scripts is
detailed along this documentation.

**4. TLDR.** We provide a script for full reproduction of each of the four
experiments. They are located in the ```scripts/``` directory, with the name
```run_<alg>_fia_simulation_data_generation_<atk_name>.sh```, where ```<alg>```
is either ```bike``` or ```hqc```, and ```<atk_name>``` is the name of
the considered attack (i.e., ```bikeflip``` and ```bgf``` for the attacks on
the corresponding BIKE's decoder version, and ```schoolbook_1fault```
and ```vect_add_1fault``` are the attacks on the polynomial multiplication
and the vector addition of HQC, correspondingly)

## Table of Contents

* Overview of this package
* Reproducing the results
  * System setup and dependencies
  * The main script for generating the dataset
* Short guide to the code
  * Compilation and dataset reproduction
    * Attack on HQC's polynomial multiplication
    * Attack on HQC's vector addition
    * Attack on BIKE's BGF threshold
    * Attack on BIKE-flip iteration
  * Running the attacks

## Overview of this package

* ```README.md```: this file.
* ```requirements.txt```: the packages needed to create the Python3 environment
to reproduce the attacks.
* ```spec_rec.py```: the Python3 script to reproduce the attack.
* ```analysis/```:
* ```bike/```, ```bike_arm_asm_faulted_threshold```, ```bike-new-bikeflip/```,
and ```hqc/```: Source codes containing the implementation of BIKE and HQC.
In BIKE's case, the first two directories contain the code associated
with the BGF decoder, and the last one is associated to
BIKE's present version with a bit flipping decoder.
* ```graphs/```: Python3 script to generate all plots used in the paper.
* ```results/```: The experiments' results.
* ```scripts/```: Directory with scripts to reproduce the attacks based on data
available in ```results/```.

## Reproducing the results

First, we list the necessary dependencies, and then we introduce the scripts for
the complete reproduction.

### Tools

To reproduce our experiments, we make use of the following tools:

1. CMake version 3.50 (or higher).
2. gcc
3. Python3 and the dependencies listed in ```requirements.txt```.

### The main script for generating the dataset

It is important to notice that to generate all the data, it would
take, approximately, 24 hours on a commodity device.
Inside the ```scripts/``` directory, there is one script associated to each of
the attacks. These correspond to the ```run_<...>.sh``` scripts.
As an example, consider the following to generate all the data associated
with HQC's fault injection attack in the vector addition.

```terminal
./scripts/run_hqc_fia_simulation_data_generation_vect_add_1fault.sh
```

Executing this script would generate the csv data in the ```results/```
directory. Analogously, for the other attacks, the procedure is very
much the same (in general lines).

## Short guide to the code

### Compilation and dataset reproduction

To compile the code, run the following from the project's root directory:

```terminal
cmake -B build
```

Once you finish, you will have to run the corresponding ```make``` command for
BIKE and HQC, depending on the attack.

### Attack on HQC's polynomial multiplication

The executable is located at:

```terminal
$(pwd)/build/hqc/fia_simulation/
```

Under the name

```terminal
fia_simulation_cbz_hqc_1
```

And with usage as follows:

```terminal
Usage: ./fia_simulation_cbz_hqc_1 <seed>
```

### Attack on HQC's vector addition

The executable is located at:

```terminal
$(pwd)/build/hqc/fia_simulation
```

Under the name

```terminal
fia_simulation_hqc_1
```

And with usage as follows:

```terminal
Usage: ./fia_simulation_hqc_1 <n_decaps>
```

The corresponding results are located at ```$(pwd)/results/hqc/1fault_vect_add/```.

### Attack on BIKE's BGF threshold

You can reproduce this (and the other attacks) by simply running the
corresponding scripts in the ```scripts/``` directory.
Here, we give a more detailed description for the compilation guide.
To run this attack, execute the following command:

```terminal
make -C build 
```

An executable named ```bike-fia-simulation``` will be located in ```fia_on_code_kems/bike/build```.

```terminal
$ ./bike/build/bike-fia-simulation
Usage: ./bike-fia-simulation <seed> <n_decaps>
```

Where ```seed``` corresponds to the seed value, and ```n_decaps``` to the number
of decapsulations we would like to simulate.
An example of output is shown next (file ```fia_on_code_kems/results/bike/bike-results-original/bike1_seed1_FIAerror0.004_5M.csv```):

```terminal
n_challenges,distance,spec_h0,spec_e0_estimator,spec_h1,spec_e1_estimator
100000,1,2,0.6365,0,0.6422
100000,2,1,0.6422,1,0.6397
100000,3,1,0.6460,0,0.6418
100000,4,1,0.6408,1,0.6387
100000,5,1,0.6440,1,0.6400
100000,6,0,0.6446,0,0.6408
100000,7,1,0.6411,0,0.6450
100000,8,0,0.6447,0,0.6419
100000,9,0,0.6447,0,0.6429
100000,10,1,0.6391,0,0.6408
100000,11,1,0.6406,0,0.6433
100000,12,0,0.6483,0,0.6438
100000,13,0,0.6410,0,0.6428
100000,14,0,0.6418,0,0.6404
100000,15,0,0.6427,0,0.6432
100000,16,1,0.6418,0,0.6414
100000,17,0,0.6408,0,0.6445
100000,18,0,0.6463,1,0.6421
100000,19,0,0.6406,0,0.6421
```

### Attack on BIKE-flip iteration

To run this attack, execute the following command:

```terminal
make -C bike-new-bikeflip
```

An executable named ```bike-fia-simulation``` will be located in ```fia_on_code_kems/bike/build```.

```terminal
$ ./bike-new-bikeflip/bike-flip-fia-simulation
Usage: ./bike-flip-fia-simulation <seed> <n_decaps> <encaps_base_seed>
```

An example of output is shown next (file ```fia_on_code_kems/results/bike-new-bikeflip/bike_new_bikeflip1_seed1.csv```):

```terminal
distance,spec_h0,spec_h1,n_challenges,spec_e0_failure,spec_e0_count,spec_e1_failure,spec_e1_count,spec_e0_estimator,spec_e1_estimator
1,1,0,2000000,414124,722806,421210,723351,0.5729393502544251,0.5823037501849033
2,1,1,2000000,414488,722977,421523,723846,0.5733073112975932,0.58233795586354
3,0,0,2000000,413247,721947,420436,722415,0.5724062846718665,0.5819868081365974
4,0,1,2000000,414527,723686,419910,722052,0.5727995290775281,0.581550913230626
5,1,1,2000000,414708,723767,420735,723815,0.5729855050036822,0.5812742206226729
6,2,0,2000000,412728,722843,420362,723184,0.5709787602563765,0.5812656253456935
7,0,0,2000000,414886,724711,420881,724519,0.5724847559923887,0.5809109215907381
8,0,1,2000000,415891,724646,420316,722252,0.5739229913640591,0.5819520056711508
9,0,1,2000000,414792,724102,420568,723916,0.5728364235977804,0.5809624321053823
10,1,0,2000000,413347,723119,419690,721842,0.5716168431475317,0.5814153235749652
11,0,2,2000000,414304,723651,421227,724657,0.5725190734207511,0.5812777631348348
12,0,0,2000000,413728,722384,419999,722977,0.5727258632527852,0.5809299604275101
13,0,1,2000000,414076,722215,420926,723911,0.5733417334173342,0.5814609807006662
14,1,1,2000000,415093,723862,420073,723095,0.5734421754422805,0.5809374978391497
15,1,1,2000000,415288,724230,420837,722599,0.573420046117946,0.582393554378016
16,0,0,2000000,413916,722773,421487,724186,0.5726777286921343,0.5820148414909981
17,1,0,2000000,414083,723687,421095,722651,0.5721852126679076,0.5827086657321445
18,0,1,2000000,413669,722672,421457,724964,0.5724159784798636,0.5813488669782224
19,0,0,2000000,414207,722472,420760,723932,0.5733191044081986,0.5812148102307951
```

### Running the attacks

Run the following to reproduce all the attacks:

```terminal
python3 spec_rec.py
```

The file contains the scripts for running all the attacks in the main function.
If you want to customize it by running only one of the presented attacks, simply
comment all the others in the main function.
