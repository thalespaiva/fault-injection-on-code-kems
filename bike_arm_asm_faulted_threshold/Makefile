all: thr thr-faultA thr-faultB thr_fia_ref

thr: get_threshold1.S get_threshold3.S get_threshold5.S main.c
	arm-linux-gnueabihf-gcc -O2 -march=armv7-a -marm main.c get_threshold1.S get_threshold3.S get_threshold5.S -o thr

thr-faultA: get_threshold1.S get_threshold3.S get_threshold5.S main_faultA.c
	arm-linux-gnueabihf-gcc -DFAULT_A -O2 -march=armv7-a -marm main_faultA.c get_threshold1.S get_threshold3.S get_threshold5.S -o thr-faultA

thr-faultB: get_threshold1.S get_threshold3.S get_threshold5.S main_faultB.c
	arm-linux-gnueabihf-gcc -DFAULT_B -O2 -march=armv7-a -marm main_faultB.c get_threshold1.S get_threshold3.S get_threshold5.S -o thr-faultB

thr_fia_ref: ref_fia.c
	gcc -Wall -O2 ref_fia.c -o thr_ref_fia

run: thr
	qemu-arm -L /usr/arm-linux-gnueabihf ./thr
	qemu-arm -L /usr/arm-linux-gnueabihf ./thr-faultA
	qemu-arm -L /usr/arm-linux-gnueabihf ./thr-faultB
