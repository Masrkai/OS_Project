# Compiler
CC = gcc

# ==================== SCHEDULER PROJECT ====================
# Release flags (optimized, no debug symbols)
CFLAGS_RELEASE = -O2 -Wall -Wextra

# Debug flags (no optimization, debug symbols, frame pointer for profiling)
CFLAGS_DEBUG = -O1 -g3 -Wall -Wextra -fno-omit-frame-pointer -rdynamic 

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

GTK_GUI_SRC_DIR = UI
GTK_GUI_BUILD_DIR = $(BUILD_DIR)/GUI
GTK_GUI_BUILD_OBJECT_DIR = $(GTK_GUI_BUILD_DIR)/objects

RELEASE_DIR = $(BUILD_DIR)/release
RELEASE_OBJ_DIR = $(RELEASE_DIR)/objects

DEBUG_DIR = $(BUILD_DIR)/debug
DEBUG_OBJ_DIR = $(DEBUG_DIR)/objects

TESTING_DIR = $(BUILD_DIR)/tests

SCHEDULER_DIR = $(SRC_DIR)/schedulers

# Linker flags
LDFLAGS_SCHEDULER = -lm

# ==================== TEST/GTK PROJECT ====================
CRITERION_FLAGS = $(shell pkg-config --cflags --libs criterion)
GTK4_FLAGS = $(shell pkg-config --cflags --libs gtk4)

CFLAGS_BASE = -Wall -Wextra -I.
CFLAGS_CRITERION = $(CFLAGS_BASE) $(filter -I%, $(CRITERION_FLAGS))
CFLAGS_GTK4 = $(CFLAGS_BASE) $(filter -I% -mfpmath=% -msse%, $(GTK4_FLAGS))

LDFLAGS_CRITERION = $(filter -L% -l% -Wl%, $(CRITERION_FLAGS))
LDFLAGS_GTK4 = $(filter -L% -l% -Wl%, $(GTK4_FLAGS))

# ==================== PHONY TARGETS ====================
.PHONY: all scheduler tests gtk clean clean-scheduler clean-tests help

# Default target - build everything
all: scheduler gtk tests

# Help target
help:
	@echo "Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  make all          - Build all projects (scheduler + tests + gtk)"
	@echo "  make scheduler    - Build both release and debug scheduler versions"
	@echo "  make release      - Build optimized scheduler in build/release/"
	@echo "  make debug        - Build debug scheduler in build/debug/"
	@echo "  make tests        - Build unit_tests (requires Criterion)"
	@echo "  make gtk          - Build dark GTK4 application"
	@echo ""
	@echo "Run targets:"
	@echo "  make run          - Run process_generator.out from release build"
	@echo ""
	@echo "Clean targets:"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make clean-scheduler - Remove only scheduler build artifacts"
	@echo "  make clean-tests  - Remove only test/gtk build artifacts"

# ==================== SCHEDULER TARGETS ====================
scheduler: release debug

# Release build
release: $(RELEASE_DIR) $(RELEASE_OBJ_DIR)
	@echo "Building scheduler release version..."
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/clk.c -o $(RELEASE_DIR)/clk.out
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/process.c -o $(RELEASE_DIR)/process.out
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/test_generator.c -o $(RELEASE_DIR)/test_generator.out
	$(CC) $(CFLAGS_RELEASE) $(SRC_DIR)/process_generator.c -o $(RELEASE_DIR)/process_generator.out
	$(CC) $(CFLAGS_RELEASE) -c $(SRC_DIR)/scheduler.c -o $(RELEASE_OBJ_DIR)/schedule.o
	$(CC) $(CFLAGS_RELEASE) -c $(SCHEDULER_DIR)/rr_scheduler.c -o $(RELEASE_OBJ_DIR)/rr_scheduler.o
	$(CC) $(CFLAGS_RELEASE) -c $(SCHEDULER_DIR)/hpf_scheduler.c -o $(RELEASE_OBJ_DIR)/hpf_scheduler.o
	$(CC) $(CFLAGS_RELEASE) -c $(SCHEDULER_DIR)/sjn_scheduler.c -o $(RELEASE_OBJ_DIR)/sjn_scheduler.o
	$(CC) $(CFLAGS_RELEASE) -c $(SCHEDULER_DIR)/mlfq_scheduler.c -o $(RELEASE_OBJ_DIR)/mlfq_scheduler.o
	$(CC) $(CFLAGS_RELEASE) $(RELEASE_OBJ_DIR)/schedule.o $(RELEASE_OBJ_DIR)/rr_scheduler.o $(RELEASE_OBJ_DIR)/hpf_scheduler.o $(RELEASE_OBJ_DIR)/sjn_scheduler.o $(RELEASE_OBJ_DIR)/mlfq_scheduler.o -o $(RELEASE_DIR)/scheduler.out $(LDFLAGS_SCHEDULER)
	@echo "Release build complete in build/release/"

# Debug build (for flamegraphs and profiling)
debug: $(DEBUG_DIR) $(DEBUG_OBJ_DIR)
	@echo "Building scheduler debug version..."
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/clk.c -o $(DEBUG_DIR)/clk.out
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/process.c -o $(DEBUG_DIR)/process.out
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/test_generator.c -o $(DEBUG_DIR)/test_generator.out
	$(CC) $(CFLAGS_DEBUG) $(SRC_DIR)/process_generator.c -o $(DEBUG_DIR)/process_generator.out
	$(CC) $(CFLAGS_DEBUG) -c $(SRC_DIR)/scheduler.c -o $(DEBUG_OBJ_DIR)/schedule.o
	$(CC) $(CFLAGS_DEBUG) -c $(SCHEDULER_DIR)/rr_scheduler.c -o $(DEBUG_OBJ_DIR)/rr_scheduler.o
	$(CC) $(CFLAGS_DEBUG) -c $(SCHEDULER_DIR)/hpf_scheduler.c -o $(DEBUG_OBJ_DIR)/hpf_scheduler.o
	$(CC) $(CFLAGS_DEBUG) -c $(SCHEDULER_DIR)/sjn_scheduler.c -o $(DEBUG_OBJ_DIR)/sjn_scheduler.o
	$(CC) $(CFLAGS_DEBUG) -c $(SCHEDULER_DIR)/mlfq_scheduler.c -o $(DEBUG_OBJ_DIR)/mlfq_scheduler.o
	$(CC) $(CFLAGS_DEBUG) $(DEBUG_OBJ_DIR)/schedule.o $(DEBUG_OBJ_DIR)/rr_scheduler.o $(DEBUG_OBJ_DIR)/hpf_scheduler.o $(DEBUG_OBJ_DIR)/sjn_scheduler.o $(DEBUG_OBJ_DIR)/mlfq_scheduler.o -o $(DEBUG_DIR)/scheduler.out $(LDFLAGS_SCHEDULER)
	@echo "Debug build complete in build/debug/"

# Create directories
$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

$(RELEASE_OBJ_DIR):
	mkdir -p $(RELEASE_OBJ_DIR)

$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

$(DEBUG_OBJ_DIR):
	mkdir -p $(DEBUG_OBJ_DIR)

# Run the process generator from release build
run:
	./$(RELEASE_DIR)/process_generator.out

# ==================== TEST TARGETS ====================
tests: $(TESTING_DIR)/test_clock $(TESTING_DIR)/test_process $(TESTING_DIR)/test_scheduler

$(TESTING_DIR)/test_clock: $(TEST_DIR)/test_clk.c | $(TESTING_DIR)
	$(CC) $(CFLAGS_CRITERION) $< -o $@ $(LDFLAGS_CRITERION)

$(TESTING_DIR)/test_process: $(TEST_DIR)/test_process.c | $(TESTING_DIR)
	$(CC) $(CFLAGS_CRITERION) $< -o $@ $(LDFLAGS_CRITERION)

$(TESTING_DIR)/test_scheduler: $(TEST_DIR)/test_scheduler.c | $(TESTING_DIR)
	$(CC) $(CFLAGS_CRITERION) $< -o $@ $(LDFLAGS_CRITERION)

$(TESTING_DIR):
	mkdir -p $@

# ==================== GTK TARGETS ====================

$(GTK_GUI_BUILD_DIR):
	mkdir -p $@

$(GTK_GUI_BUILD_OBJECT_DIR):
	mkdir -p $@

# Explicitly define the single source and object
GTK_SOURCE = $(GTK_GUI_SRC_DIR)/dark.c
GTK_OBJECT = $(GTK_GUI_BUILD_OBJECT_DIR)/dark.o
GTK_TARGET = $(GTK_GUI_BUILD_DIR)/dark

gtk: $(GTK_TARGET)

$(GTK_OBJECT): $(GTK_SOURCE) | $(GTK_GUI_BUILD_OBJECT_DIR)
	$(CC) $(CFLAGS_GTK4) -c $< -o $@

$(GTK_TARGET): $(GTK_OBJECT) | $(GTK_GUI_BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS_GTK4)

# ==================== CLEAN TARGETS ====================
clean: clean-scheduler clean-tests

clean-scheduler:
	rm -rf $(BUILD_DIR)
	rm -f processes.txt

clean-tests:
	rm -f *.o unit_tests dark