all: thr thr_fia_ref

# thr: get_threshold.s main.c
# 	arm-linux-gnueabi-gcc main.c get_threshold.s -O3 -o thr

thr: get_threshold.s main.c
	arm-linux-gnueabihf-gcc -O2 -march=armv7-a -marm main.c get_threshold1.s get_threshold3.s get_threshold5.s -o thr

thr_fia_ref: ref_fia.c
	gcc -Wall -O2 ref_fia.c -o thr_ref_fia

run: thr
	qemu-arm -L /usr/arm-linux-gnueabihf ./thr
