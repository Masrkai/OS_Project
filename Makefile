# Compiler and flags
CC = gcc

# Release flags (optimized, no debug symbols)
CFLAGS_RELEASE = -O2 -Wall -Wextra

# Debug flags (no optimization, debug symbols, frame pointer for profiling)
CFLAGS_DEBUG = -O0 -g3 -Wall -Wextra -fno-omit-frame-pointer -rdynamic

# Directories
BUILD_DIR = build
RELEASE_DIR = $(BUILD_DIR)/release
DEBUG_DIR = $(BUILD_DIR)/debug
RELEASE_OBJ_DIR = $(RELEASE_DIR)/objects
DEBUG_OBJ_DIR = $(DEBUG_DIR)/objects
SRC_DIR = src
SCHEDULER_DIR = $(SRC_DIR)/schedulers

# Linker flags
LDFLAGS = -lm

.PHONY: all build clean run release debug help

# Default target - build both release and debug
all: release debug

# Help target
help:
	@echo "Available targets:"
	@echo "  make all        - Build both release and debug versions"
	@echo "  make release    - Build optimized version in build/release/"
	@echo "  make debug      - Build debug version in build/debug/"
	@echo "  make clean      - Remove all build artifacts"
	@echo "  make run        - Run process_generator.out from release build"

# Release build
release: $(RELEASE_DIR) $(RELEASE_OBJ_DIR)
	@echo "Building release version..."
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/clk.c -o $(RELEASE_DIR)/clk.out
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/process.c -o $(RELEASE_DIR)/process.out
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/test_generator.c -o $(RELEASE_DIR)/test_generator.out
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/process_generator.c -o $(RELEASE_DIR)/process_generator.out
	$(CC) $(CFLAGS_RELEASE) -c $(SRC_DIR)/scheduler.c -o $(RELEASE_OBJ_DIR)/schedule.o
	$(CC) $(CFLAGS_RELEASE) -c $(SCHEDULER_DIR)/rr_scheduler.c -o $(RELEASE_OBJ_DIR)/rr_scheduler.o
	$(CC) $(CFLAGS_RELEASE) -c $(SCHEDULER_DIR)/hpf_scheduler.c -o $(RELEASE_OBJ_DIR)/hpf_scheduler.o
	$(CC) $(CFLAGS_RELEASE) -c $(SCHEDULER_DIR)/sjn_scheduler.c -o $(RELEASE_OBJ_DIR)/sjn_scheduler.o
	$(CC) $(CFLAGS_RELEASE) $(RELEASE_OBJ_DIR)/schedule.o $(RELEASE_OBJ_DIR)/rr_scheduler.o $(RELEASE_OBJ_DIR)/hpf_scheduler.o $(RELEASE_OBJ_DIR)/sjn_scheduler.o -o $(RELEASE_DIR)/scheduler.out $(LDFLAGS)
	@echo "Release build complete in build/release/"

# Debug build (for flamegraphs and profiling)
debug: $(DEBUG_DIR) $(DEBUG_OBJ_DIR)
	@echo "Building debug version..."
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/clk.c -o $(DEBUG_DIR)/clk.out
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/process.c -o $(DEBUG_DIR)/process.out
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/test_generator.c -o $(DEBUG_DIR)/test_generator.out
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/process_generator.c -o $(DEBUG_DIR)/process_generator.out
	$(CC) $(CFLAGS_DEBUG) -c $(SRC_DIR)/scheduler.c -o $(DEBUG_OBJ_DIR)/schedule.o
	$(CC) $(CFLAGS_DEBUG) -c $(SCHEDULER_DIR)/rr_scheduler.c -o $(DEBUG_OBJ_DIR)/rr_scheduler.o
	$(CC) $(CFLAGS_DEBUG) -c $(SCHEDULER_DIR)/hpf_scheduler.c -o $(DEBUG_OBJ_DIR)/hpf_scheduler.o
	$(CC) $(CFLAGS_DEBUG) -c $(SCHEDULER_DIR)/sjn_scheduler.c -o $(DEBUG_OBJ_DIR)/sjn_scheduler.o
	$(CC) $(CFLAGS_DEBUG) $(DEBUG_OBJ_DIR)/schedule.o $(DEBUG_OBJ_DIR)/rr_scheduler.o $(DEBUG_OBJ_DIR)/hpf_scheduler.o $(DEBUG_OBJ_DIR)/sjn_scheduler.o -o $(DEBUG_DIR)/scheduler.out $(LDFLAGS)
	@echo "Debug build complete in build/debug/"
	@echo "To generate flamegraphs, use:"
	@echo "  perf record -F 99 -g ./build/debug/scheduler.out <args>"
	@echo "  perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg"

# Create directories
$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

$(RELEASE_OBJ_DIR):
	mkdir -p $(RELEASE_OBJ_DIR)

$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

$(DEBUG_OBJ_DIR):
	mkdir -p $(DEBUG_OBJ_DIR)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f processes.txt

# Run the process generator from release build
run:
	./$(RELEASE_DIR)/process_generator.out