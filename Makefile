build:
	mkdir -p build
	mkdir -p build/objects

	gcc src/clk.c               -o build/clk.out
	gcc src/process.c           -o build/process.out
	gcc src/test_generator.c    -o build/test_generator.out
	gcc src/process_generator.c -o build/process_generator.out

	gcc -c src/scheduler.c                -o build/objects/schedule.o
	gcc -c src/schedulers/rr_scheduler.c  -o build/objects/rr_scheduler.o
	gcc -c src/schedulers/hpf_scheduler.c -o build/objects/hpf_scheduler.o
	gcc -c src/schedulers/sjn_scheduler.c -o build/objects/sjn_scheduler.o

#! <math.h> the math library (libm) doesn't get linked on most Unix toolchains unless you pass `-lm` to the linker.
	gcc \
	 build/objects/schedule.o      \
	 build/objects/rr_scheduler.o  \
	 build/objects/hpf_scheduler.o \
	 build/objects/sjn_scheduler.o \
	-o build/scheduler.out -lm


clean:
	rm -f build/*.out processes.txt

all: clean build

run:
	./build/process_generator.out
