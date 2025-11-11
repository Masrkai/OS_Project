build:
	gcc src/process_generator.c -o process_generator.out
	gcc src/clk.c -o clk.out

#! <math.h> the math library (libm)
#! doesn't get linked on most Unix toolchains
#! unless you pass `-lm` to the linker.
	gcc src/scheduler.c -o scheduler.out -lm
	gcc src/process.c -o process.out
	gcc src/test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out
