# Build System & Compilation Process

## Overview

This project uses **GNU Make** as its build system, providing multiple build configurations for different purposes: optimized releases, debug builds with profiling support, unit tests, and a GTK4 GUI application.

## Build System Architecture

```
Project Root
├── Makefile (main build configuration)
├── src/                    (scheduler source files)
│   ├── schedulers/        (algorithm implementations)
│   └── *.c                (main components)
├── test/                  (unit test files)
├── UI/                    (GTK4 GUI source)
└── build/                 (generated build outputs)
    ├── release/           (optimized binaries)
    │   └── objects/       (release .o files)
    ├── debug/             (debug binaries with symbols)
    │   └── objects/       (debug .o files)
    ├── tests/             (test executables)
    └── GUI/               (GTK4 application)
        └── objects/       (GTK4 .o files)
```

## Compiler Configuration

### Base Compiler

```makefile
CC = gcc
```

**Why GCC**: Standard C compiler with excellent optimization and debugging support

### Release Flags

```makefile
CFLAGS_RELEASE = -O2 -Wall -Wextra
```

**Flag Breakdown**:

- `-O2`: Optimization level 2 (balanced speed/size)
  - Enables: function inlining, loop optimization, dead code elimination
  - Typical performance gain: 30-50% faster than -O0
- `-Wall`: Enable all common warnings
- `-Wextra`: Enable extra warnings (unused parameters, sign comparisons, etc.)

**Use Case**: Production builds, normal execution

### Debug Flags

```makefile
CFLAGS_DEBUG = -O0 -g3 -Wall -Wextra -fno-omit-frame-pointer -rdynamic
```

**Flag Breakdown**:

- `-O0`: No optimization (preserve exact code flow)
  - Makes debugging predictable (no reordered statements)
- `-g3`: Maximum debug information
  - Includes macro definitions for debugger
  - Enables source-level debugging
- `-fno-omit-frame-pointer`: Keep frame pointer in register
  - **Critical for profiling**: Allows stack unwinding
  - Enables accurate flamegraphs
- `-rdynamic`: Export all symbols for dynamic linking
  - Makes function names visible to profilers
  - Useful for backtrace() calls

**Use Case**: Development, debugging, performance profiling with `perf`

### Linker Flags

```makefile
LDFLAGS_SCHEDULER = -lm
```

**Why `-lm`**: Links math library for `sqrt()` function used in standard deviation calculation

## Build Targets Hierarchy

### Master Targets

#### `make all`

**Purpose**: Build everything (default target)

**Execution Order**:

1. `make scheduler` (both release and debug)
2. `make gtk` (GTK4 application)
3. `make tests` (unit tests)

**When to Use**: Initial build, full rebuild after major changes

#### `make help`

**Purpose**: Display available targets and usage

**Output**: Formatted help text explaining all targets

### Scheduler Build Targets

#### `make scheduler`

**Purpose**: Build both release and debug versions

**Dependencies**: Calls `make release` and `make debug`

**Result**: Two complete sets of binaries in separate directories

#### `make release`

**Purpose**: Build optimized production binaries

**Build Process**:

1. **Create Directories**

   ```bash
   mkdir -p build/release
   mkdir -p build/release/objects
   ```

2. **Compile Standalone Executables** (direct source → binary)

   ```makefile
   gcc -O2 -Wall -Wextra src/clk.c -o build/release/clk.out
   gcc -O2 -Wall -Wextra src/process.c -o build/release/process.out
   gcc -O2 -Wall -Wextra src/test_generator.c -o build/release/test_generator.out
   gcc -O2 -Wall -Wextra src/process_generator.c -o build/release/process_generator.out
   ```

   **Why Direct Compilation**: These are simple programs with no shared dependencies

3. **Compile Scheduler Components** (source → object files)

   ```makefile
   gcc -O2 -Wall -Wextra -c src/scheduler.c -o build/release/objects/schedule.o
   gcc -O2 -Wall -Wextra -c src/schedulers/rr_scheduler.c -o build/release/objects/rr_scheduler.o
   gcc -O2 -Wall -Wextra -c src/schedulers/hpf_scheduler.c -o build/release/objects/hpf_scheduler.o
   gcc -O2 -Wall -Wextra -c src/schedulers/sjn_scheduler.c -o build/release/objects/sjn_scheduler.o
   gcc -O2 -Wall -Wextra -c src/schedulers/mlfq_scheduler.c -o build/release/objects/mlfq_scheduler.o
   ```

   **Flag Explanation**:
   - `-c`: Compile only (no linking), produces `.o` object file
   - Object files contain compiled machine code but are not executable

4. **Link Scheduler** (object files → executable)

   ```makefile
   gcc -O2 -Wall -Wextra \
       build/release/objects/schedule.o \
       build/release/objects/rr_scheduler.o \
       build/release/objects/hpf_scheduler.o \
       build/release/objects/sjn_scheduler.o \
       build/release/objects/mlfq_scheduler.o \
       -o build/release/scheduler.out \
       -lm
   ```

   **Why Separate Compilation**:
   - Modularity: Each algorithm in separate file
   - Faster rebuilds: Only recompile changed files
   - Better organization: Clear separation of concerns

**Output Location**: `build/release/`

- `clk.out` (clock process)
- `process.out` (simulated process)
- `test_generator.out` (test case generator)
- `process_generator.out` (main coordinator)
- `scheduler.out` (scheduler with all algorithms)

#### `make debug`

**Purpose**: Build with debugging symbols and profiling support

**Build Process**: Identical to release but with `CFLAGS_DEBUG`

**Key Differences**:

- No optimization: Code matches source line-by-line
- Debug symbols: Source file names, line numbers, variable names included
- Frame pointers: Stack traces work correctly
- Symbol export: Function names visible to external tools

**Output Location**: `build/debug/`

### Run Target

#### `make run`

**Purpose**: Execute the release build process generator

**Command**: `./build/release/process_generator.out`

**Requirements**: Must run `make release` first

**Interactive Prompts**:

1. Reads `processes.txt`
2. Asks for scheduling algorithm
3. Asks for quantum (if needed)
4. Executes simulation

## Test Targets

### `make tests`

**Purpose**: Build unit tests using Criterion framework

**Dependencies**: Requires Criterion library installed

**Build Process**:

1. **Extract Criterion Flags**

   ```makefile
   CRITERION_FLAGS = $(shell pkg-config --cflags --libs criterion)
   CFLAGS_CRITERION = $(CFLAGS_BASE) $(filter -I%, $(CRITERION_FLAGS))
   LDFLAGS_CRITERION = $(filter -L% -l% -Wl%, $(CRITERION_FLAGS))
   ```

   **pkg-config**: Tool that queries library metadata
   - `--cflags`: Include paths and compilation flags
   - `--libs`: Library paths and linker flags
   - `filter`: Extracts specific flag types

2. **Compile Test Executables**

   ```makefile
   gcc $(CFLAGS_CRITERION) test/test_clk.c -o build/tests/test_clock $(LDFLAGS_CRITERION)
   gcc $(CFLAGS_CRITERION) test/test_process.c -o build/tests/test_process $(LDFLAGS_CRITERION)
   ```

**Test Files**:

- `test_clock`: Tests clock initialization and shared memory
- `test_process`: Tests process execution and signal handling

**Note**: `test_scheduler` is commented out (work in progress)

## GTK4 GUI Target

### `make gtk`

**Purpose**: Build GTK4 graphical interface

**Dependencies**: Requires GTK4 library installed

**Build Process**:

1. **Extract GTK4 Flags**

   ```makefile
   GTK4_FLAGS = $(shell pkg-config --cflags --libs gtk4)
   CFLAGS_GTK4 = $(CFLAGS_BASE) $(filter -I% -mfpmath=% -msse%, $(GTK4_FLAGS))
   LDFLAGS_GTK4 = $(filter -L% -l% -Wl%, $(GTK4_FLAGS))
   ```

   **Special Flags**:
   - `-mfpmath=sse`: Use SSE for floating-point math (performance)
   - `-msse`: Enable SSE instruction set

2. **Create Build Directories**

   ```makefile
   mkdir -p build/GUI
   mkdir -p build/GUI/objects
   ```

3. **Compile GUI Source**

   ```makefile
   gcc $(CFLAGS_GTK4) -c UI/dark.c -o build/GUI/objects/dark.o
   ```

4. **Link GUI Application**

   ```makefile
   gcc build/GUI/objects/dark.o -o build/GUI/dark $(LDFLAGS_GTK4)
   ```

**Output**: `build/GUI/dark` (executable GUI application)

**Why Separate Build**: GTK4 has complex dependencies (glib, cairo, pango) that don't apply to scheduler

## Clean Targets

### `make clean`

**Purpose**: Remove all build artifacts

**Actions**:

1. Calls `make clean-scheduler`
2. Calls `make clean-tests`

**Complete Cleanup**: Ensures fresh build on next `make`

### `make clean-scheduler`

**Purpose**: Remove scheduler build outputs

**Actions**:

```bash
rm -rf build/
rm -f processes.txt
```

### `make clean-tests`

**Purpose**: Remove test and GTK build artifacts

**Actions**:

```bash
rm -f *.o unit_tests dark compile_commands.json
```

**Files Removed**:

- `.o`: Stray object files in root
- `unit_tests`: Old test executable
- `dark`: Old GTK executable (if built in root)
- `compile_commands.json`: LSP compilation database

## Compilation Stages Explained

### Stage 1: Preprocessing

**Command**: `gcc -E source.c`

**Actions**:

- Include header files (`#include`)
- Expand macros (`#define`)
- Process conditional compilation (`#ifdef`)
- Remove comments

**Output**: Expanded source code (still C, just larger)

### Stage 2: Compilation

**Command**: `gcc -S source.c` (or implicit with `-c`)

**Actions**:

- Parse C syntax
- Perform semantic analysis
- Optimize code (if optimization enabled)
- Generate assembly code

**Output**: Assembly language file (`.s`)

### Stage 3: Assembly

**Command**: `gcc -c source.c` or `as source.s`

**Actions**:

- Convert assembly to machine code
- Create symbol table (function/variable names)
- Generate relocation information

**Output**: Object file (`.o`) - binary but not executable

### Stage 4: Linking

**Command**: `gcc obj1.o obj2.o -o program`

**Actions**:

- Resolve external symbol references
- Combine object files
- Link system libraries
- Set entry point
- Create executable format (ELF on Linux)

**Output**: Executable binary

## Understanding Object Files vs Executables

### Object Files (.o)

```
schedule.o contains:
- Compiled machine code for scheduler.c functions
- Symbol table: initQueue, enqueue, dequeue, etc.
- Unresolved references: selectHPF (from hpf_scheduler.o)
```

### Linking Process

```
Linker combines:
  schedule.o         (provides: main, enqueue, dequeue)
  + rr_scheduler.o   (provides: selectRR)
  + hpf_scheduler.o  (provides: selectHPF)
  + sjn_scheduler.o  (provides: selectSJN)
  + mlfq_scheduler.o (provides: selectMLFQ, initMLFQ)
  + libc.so          (provides: printf, malloc, fork)
  + libm.so          (provides: sqrt)
→ scheduler.out (executable with all symbols resolved)
```

## Dependency Management

### Implicit Dependencies

Make tracks file modification times:

```
If scheduler.c newer than schedule.o:
  → Recompile schedule.o
  → Relink scheduler.out

If only hpf_scheduler.c changed:
  → Recompile only hpf_scheduler.o
  → Relink scheduler.out
  → Don't recompile other .o files
```

**Benefit**: Faster incremental builds

### Directory Dependencies

```makefile
$(RELEASE_DIR):
    mkdir -p $(RELEASE_DIR)
```

**Pipe `|` Operator** (Order-only dependency):

```makefile
target: dependencies | order-only-dependencies
```

**Example**:

```makefile
build/release/clk.out: src/clk.c | $(RELEASE_DIR)
```

Means: Build directory must exist, but don't rebuild if directory timestamp changes

## Optimization Levels Comparison

### `-O0` (Debug)

- No optimization
- Fastest compilation
- Largest binaries
- Easiest debugging
- Slowest execution

### `-O1` (Basic)

- Simple optimizations
- Minimal code size reduction
- Debugging still reasonable
- Moderate speed improvement

### `-O2` (Release - Used in Project)

- Aggressive optimization
- Function inlining
- Loop unrolling
- Common subexpression elimination
- Significant speed improvement (30-50%)
- Debugging difficult

### `-O3` (Maximum)

- All `-O2` optimizations
- Aggressive loop transformations
- Automatic vectorization
- Largest code size
- Highest speed (10-20% over -O2)

### `-Os` (Size)

- Optimize for size
- Useful for embedded systems

### `-Og` (Debug with optimization)

- Optimize while preserving debuggability
- Good compromise for development

## Build Automation Flow

### Initial Build

```bash
make all
```

**Execution Sequence**:

```
1. make scheduler
   ├─ make release
   │  ├─ mkdir -p build/release/objects
   │  ├─ compile clk.c → clk.out
   │  ├─ compile process.c → process.out
   │  ├─ compile test_generator.c → test_generator.out
   │  ├─ compile process_generator.c → process_generator.out
   │  ├─ compile scheduler.c → objects/schedule.o
   │  ├─ compile rr_scheduler.c → objects/rr_scheduler.o
   │  ├─ compile hpf_scheduler.c → objects/hpf_scheduler.o
   │  ├─ compile sjn_scheduler.c → objects/sjn_scheduler.o
   │  ├─ compile mlfq_scheduler.c → objects/mlfq_scheduler.o
   │  └─ link all objects → scheduler.out
   └─ make debug
      └─ (same as release with debug flags)

2. make gtk
   ├─ mkdir -p build/GUI/objects
   ├─ compile dark.c → objects/dark.o
   └─ link dark.o → GUI/dark

3. make tests
   ├─ mkdir -p build/tests
   ├─ compile test_clk.c → tests/test_clock
   └─ compile test_process.c → tests/test_process
```

### Incremental Build

```bash
# Modify only hpf_scheduler.c
make release
```

**Smart Rebuild**:

```
✓ Skip: clk.c (unchanged)
✓ Skip: process.c (unchanged)
✓ Skip: test_generator.c (unchanged)
✓ Skip: process_generator.c (unchanged)
✓ Skip: scheduler.c (unchanged)
✓ Skip: rr_scheduler.c (unchanged)
✗ Rebuild: hpf_scheduler.c (modified) → hpf_scheduler.o
✓ Skip: sjn_scheduler.c (unchanged)
✓ Skip: mlfq_scheduler.c (unchanged)
✗ Relink: scheduler.out (dependency changed)
```

**Time Saved**: ~90% faster than full rebuild

## External Library Integration

### pkg-config Mechanism

**Purpose**: Provides compiler/linker flags for installed libraries

**Example Query**:

```bash
pkg-config --cflags --libs gtk4
```

**Output**:

```
-I/usr/include/gtk-4.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
-L/usr/lib -lgtk-4 -lgdk-4 -lgobject-2.0 -lglib-2.0
```

**Makefile Integration**:

```makefile
GTK4_FLAGS = $(shell pkg-config --cflags --libs gtk4)
```

**Benefits**:

- Platform-independent (paths differ across systems)
- Version-agnostic (adapts to installed version)
- Comprehensive (includes all dependencies)

## Phony Targets

```makefile
.PHONY: all scheduler tests gtk clean clean-scheduler clean-tests help
```

**Purpose**: Declare targets that don't represent files

**Why Needed**: If file named `clean` existed, `make clean` wouldn't run

**Convention**: Always declare non-file targets as phony

## Best Practices Demonstrated

### 1. Separate Build Directories

**Benefit**: Clean separation of release/debug/test builds

### 2. Object File Caching

**Benefit**: Fast incremental builds

### 3. Modular Compilation

**Benefit**: Each algorithm in separate file, easy to add/remove

### 4. Flag Isolation

**Benefit**: Different flags for different targets (release/debug/test)

### 5. Automatic Directory Creation

**Benefit**: No manual setup required

### 6. Help Target

**Benefit**: Self-documenting Makefile

### 7. Clean Targets

**Benefit**: Easy reset to pristine state

## Common Build Issues & Solutions

### Issue: "command not found"

**Cause**: Library not installed
**Solution**: `sudo apt install libgtk-4-dev criterion-dev`

### Issue: Linking errors

**Cause**: Object files out of date
**Solution**: `make clean && make`

### Issue: Permission denied

**Cause**: Executable bit not set
**Solution**: `chmod +x build/release/*.out`

### Issue: Shared memory errors at runtime

**Cause**: Previous run didn't clean up
**Solution**: Remove `/dev/shm/*` files or reboot

## Development Workflow

### Standard Development Cycle

```bash
# 1. Clean build
make clean
make debug

# 2. Test
cd build/debug
./test_generator.out  # Generate test processes
./process_generator.out  # Run simulation

# 3. Profile (if needed)
perf record -F 99 -g ./scheduler.out 1 2 <msgqid>
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg

# 4. View logs
cat scheduler.log
cat scheduler.perf

# 5. Optimize code based on profile

# 6. Build release
cd ../..
make release

# 7. Benchmark
time ./build/release/process_generator.out
```

### Quick Test Cycle

```bash
make release && cd build/release && ./test_generator.out && ./process_generator.out
```

## Summary

This build system provides:

- **Two build configurations**: Optimized release and debuggable profiling builds
- **Modular compilation**: Fast incremental builds
- **Multiple projects**: Scheduler, tests, GUI in one Makefile
- **Platform independence**: Uses pkg-config for library detection
- **Developer-friendly**: Help target, automatic directory creation, clean targets

The separation of release and debug builds allows efficient development while ensuring production performance.
