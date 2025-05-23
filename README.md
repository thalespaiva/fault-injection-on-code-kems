# Fault injection attacks on code-based schemes


## Compiling

```bash
$ cmake -B build
$ make -C build
```

## Running key recovery experiments


Usage:
```bash
$ ./build/key_recovery/key_recovery
Usage: <r> <w> <ntests> <n_out_spectrum> <n_tries> <seed> <nthreads> [out_spec_step, out_spec_lim]
```

Simulating a BIKE key recovery with 4000 known entries outside the spectrum.

```bash
$ ./build/key_recovery/key_recovery 12323 67 1 4000 100 0 1
=============== New battery n_out_spec = 4000 ============

------------ Starting Test    1 ---------------
Targeting key:
134 372 463 479 1197 1248 1286 1631 2041 2053 2372 2628 2700 2734 3264 3473 3647 4134 4223 4259 4463 4883 5015 5106 5732 5895 5896 6544 6670 6705 6737 6820 6824 7272 7554 7818 7891 7903 7949 8059 8065 8426 8720 8865 9074 9511 9936 10086 10121 10153 10251 10345 10526 10800 10871 10873 10913 11076 11090 11173 11209 11423 11491 12070 12158 12193 12250 [.] # possible positions = 1570
[-] Failed
[+] wall time = 0.0305963730000000 s
[+] cpu time = 0.0300792970000000 s
[-] Stopping Attack. Try a larger number of constructions
-----------------------------------------------

==========================================================
```


## Running simulation on the skipping fault on the 2nd iteration

Usage:
```bash
$ ./build/bike/bike-fia-simulation
Usage: ./build/bike/bike-fia-simulation <seed> <n_decaps>

```

Seeing the impact of the skipping fault in 10 decapsulations.
```
$ ./build/bike/bike-fia-simulation 1 10
tmp_syndrome_weight,decaps_failed
3322,1
3038,0
3445,1
3411,1
3187,0
3274,1
3340,1
3317,1
3492,1
3461,1
```

