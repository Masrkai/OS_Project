build:
	mkdir -p build

	gcc src/process_generator.c -o build/process_generator.out
	gcc src/clk.c -o build/clk.out

#! <math.h> the math library (libm)
#! doesn't get linked on most Unix toolchains
#! unless you pass `-lm` to the linker.
	gcc src/scheduler.c -o build/scheduler.out -lm
	gcc src/process.c -o build/process.out
	gcc src/test_generator.c -o build/test_generator.out

clean:
	rm -f build/*.out processes.txt

all: clean build

run:
	./build/process_generator.out
