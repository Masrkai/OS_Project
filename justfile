# ==================== VARIABLES ====================

# Compiler
CC := "gcc"

# Directories
SRC_DIR := "src"
TEST_DIR := "test"
BUILD_DIR := "build"

GTK_GUI_SRC_DIR := "UI"
GTK_GUI_BUILD_DIR := BUILD_DIR + "/GUI"
GTK_GUI_BUILD_OBJECT_DIR := GTK_GUI_BUILD_DIR + "/objects"

RELEASE_DIR := BUILD_DIR + "/release"
RELEASE_OBJ_DIR := RELEASE_DIR + "/objects"

DEBUG_DIR := BUILD_DIR + "/debug"
DEBUG_OBJ_DIR := DEBUG_DIR + "/objects"

TESTING_DIR := BUILD_DIR + "/tests"

SCHEDULER_DIR := SRC_DIR + "/schedulers"

# Scheduler Flags
CFLAGS_RELEASE := "-O2 -Wall -Wextra"
CFLAGS_DEBUG := "-O1 -g3 -Wall -Wextra -fno-omit-frame-pointer -fno-optimize-sibling-calls -rdynamic"
LDFLAGS_SCHEDULER := "-lm"

# Base & External Library Flags
CFLAGS_BASE := "-Wall -Wextra -I."

# Note: We use `|| echo ""` to prevent just from failing to parse if the library isn't installed yet.
CFLAGS_CRITERION := CFLAGS_BASE + " " + `pkg-config --cflags criterion 2>/dev/null || echo ""`
LDFLAGS_CRITERION := `pkg-config --libs criterion 2>/dev/null || echo ""`

CFLAGS_GTK4 := CFLAGS_BASE + " " + `pkg-config --cflags gtk4 2>/dev/null || echo ""`
LDFLAGS_GTK4 := `pkg-config --libs gtk4 2>/dev/null || echo ""`

CFLAGS_GTK3 := CFLAGS_BASE + " " + `pkg-config --cflags gtk+-3.0 2>/dev/null || echo ""`
LDFLAGS_GTK3 := `pkg-config --libs gtk+-3.0 2>/dev/null || echo ""`


# ==================== DEFAULT / HELP ====================

# Build all projects (scheduler + tests + gtk + gui)
all: scheduler gtk gui tests

# Print help message
help:
    @echo "Available targets:"
    @echo ""
    @echo "Build targets:"
    @echo "  just all              - Build all projects (scheduler + tests + gtk + gui)"
    @echo "  just scheduler        - Build both release and debug scheduler versions"
    @echo "  just release          - Build optimized scheduler in build/release/"
    @echo "  just debug            - Build debug scheduler in build/debug/"
    @echo "  just gui              - Build scheduler GUI (GTK3, in build/GUI/)"
    @echo "  just tests            - Build unit_tests (requires Criterion)"
    @echo "  just gtk              - Build dark GTK4 application"
    @echo ""
    @echo "Run targets:"
    @echo "  just run              - Run process_generator.out from release build"
    @echo "  just run-gui          - Run the scheduler GUI"
    @echo ""
    @echo "Install targets:"
    @echo "  just install-gtk-deps - Install GTK3 dependencies"
    @echo ""
    @echo "Clean targets:"
    @echo "  just clean            - Remove all build artifacts"
    @echo "  just clean-scheduler  - Remove only scheduler build artifacts"
    @echo "  just clean-tests      - Remove only test/gtk build artifacts"
    @echo "  just clean-gui        - Remove only GUI build artifacts"


# ==================== SCHEDULER TARGETS ====================

# Build both release and debug scheduler versions
scheduler: release debug

# Build optimized scheduler in build/release/
release:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Building scheduler release version..."
    mkdir -p {{RELEASE_DIR}} {{RELEASE_OBJ_DIR}}

    {{CC}} {{CFLAGS_RELEASE}} {{SRC_DIR}}/clk.c -o {{RELEASE_DIR}}/clk.out
    {{CC}} {{CFLAGS_RELEASE}} {{SRC_DIR}}/process.c -o {{RELEASE_DIR}}/process.out
    {{CC}} {{CFLAGS_RELEASE}} {{SRC_DIR}}/test_generator.c -o {{RELEASE_DIR}}/test_generator.out
    {{CC}} {{CFLAGS_RELEASE}} {{SRC_DIR}}/process_generator.c -o {{RELEASE_DIR}}/process_generator.out

    {{CC}} {{CFLAGS_RELEASE}} -c {{SRC_DIR}}/scheduler.c -o {{RELEASE_OBJ_DIR}}/schedule.o
    {{CC}} {{CFLAGS_RELEASE}} -c {{SCHEDULER_DIR}}/rr_scheduler.c -o {{RELEASE_OBJ_DIR}}/rr_scheduler.o
    {{CC}} {{CFLAGS_RELEASE}} -c {{SCHEDULER_DIR}}/hpf_scheduler.c -o {{RELEASE_OBJ_DIR}}/hpf_scheduler.o
    {{CC}} {{CFLAGS_RELEASE}} -c {{SCHEDULER_DIR}}/sjn_scheduler.c -o {{RELEASE_OBJ_DIR}}/sjn_scheduler.o
    {{CC}} {{CFLAGS_RELEASE}} -c {{SCHEDULER_DIR}}/mlfq_scheduler.c -o {{RELEASE_OBJ_DIR}}/mlfq_scheduler.o

    {{CC}} {{CFLAGS_RELEASE}} \
        {{RELEASE_OBJ_DIR}}/schedule.o \
        {{RELEASE_OBJ_DIR}}/rr_scheduler.o \
        {{RELEASE_OBJ_DIR}}/hpf_scheduler.o \
        {{RELEASE_OBJ_DIR}}/sjn_scheduler.o \
        {{RELEASE_OBJ_DIR}}/mlfq_scheduler.o \
        -o {{RELEASE_DIR}}/scheduler.out {{LDFLAGS_SCHEDULER}}

    echo "Release build complete in {{RELEASE_DIR}}/"

# Build debug scheduler in build/debug/ (for flamegraphs and profiling)
debug:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Building scheduler debug version..."
    mkdir -p {{DEBUG_DIR}} {{DEBUG_OBJ_DIR}}

    {{CC}} {{CFLAGS_DEBUG}} {{SRC_DIR}}/clk.c -o {{DEBUG_DIR}}/clk.out
    {{CC}} {{CFLAGS_DEBUG}} {{SRC_DIR}}/process.c -o {{DEBUG_DIR}}/process.out
    {{CC}} {{CFLAGS_DEBUG}} {{SRC_DIR}}/test_generator.c -o {{DEBUG_DIR}}/test_generator.out
    {{CC}} {{CFLAGS_DEBUG}} {{SRC_DIR}}/process_generator.c -o {{DEBUG_DIR}}/process_generator.out

    {{CC}} {{CFLAGS_DEBUG}} -c {{SRC_DIR}}/scheduler.c -o {{DEBUG_OBJ_DIR}}/schedule.o
    {{CC}} {{CFLAGS_DEBUG}} -c {{SCHEDULER_DIR}}/rr_scheduler.c -o {{DEBUG_OBJ_DIR}}/rr_scheduler.o
    {{CC}} {{CFLAGS_DEBUG}} -c {{SCHEDULER_DIR}}/hpf_scheduler.c -o {{DEBUG_OBJ_DIR}}/hpf_scheduler.o
    {{CC}} {{CFLAGS_DEBUG}} -c {{SCHEDULER_DIR}}/sjn_scheduler.c -o {{DEBUG_OBJ_DIR}}/sjn_scheduler.o
    {{CC}} {{CFLAGS_DEBUG}} -c {{SCHEDULER_DIR}}/mlfq_scheduler.c -o {{DEBUG_OBJ_DIR}}/mlfq_scheduler.o

    {{CC}} {{CFLAGS_DEBUG}} \
        {{DEBUG_OBJ_DIR}}/schedule.o \
        {{DEBUG_OBJ_DIR}}/rr_scheduler.o \
        {{DEBUG_OBJ_DIR}}/hpf_scheduler.o \
        {{DEBUG_OBJ_DIR}}/sjn_scheduler.o \
        {{DEBUG_OBJ_DIR}}/mlfq_scheduler.o \
        -o {{DEBUG_DIR}}/scheduler.out {{LDFLAGS_SCHEDULER}}

    echo "Debug build complete in {{DEBUG_DIR}}/"

# Run process_generator.out from release build
run: release
    ./{{RELEASE_DIR}}/process_generator.out


# ==================== SCHEDULER GUI TARGETS (GTK3) ====================

# Build scheduler GUI (GTK3, in build/GUI/)
gui: release
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p {{GTK_GUI_BUILD_OBJECT_DIR}} {{GTK_GUI_BUILD_DIR}}

    if ! pkg-config --exists gtk+-3.0; then
        echo ""
        echo "ERROR: GTK+3.0 not found!"
        echo "Please install with: just install-gtk-deps"
        echo "Or manually: sudo apt-get install libgtk-3-dev pkg-config"
        echo ""
        exit 1
    fi

    echo "Compiling scheduler GUI..."
    {{CC}} {{CFLAGS_GTK3}} -c {{GTK_GUI_SRC_DIR}}/scheduler_gui.c -o {{GTK_GUI_BUILD_OBJECT_DIR}}/scheduler_gui.o

    echo "Linking scheduler GUI..."
    {{CC}} {{GTK_GUI_BUILD_OBJECT_DIR}}/scheduler_gui.o -o {{GTK_GUI_BUILD_DIR}}/scheduler_gui {{LDFLAGS_GTK3}}

    echo "Creating convenience symlink..."
    rm -f scheduler_gui
    ln -sf "{{GTK_GUI_BUILD_DIR}}/scheduler_gui" scheduler_gui

    echo ""
    echo "====================================="
    echo "Scheduler GUI built successfully!"
    echo "====================================="
    echo "To run: just run-gui"
    echo "   or:  {{GTK_GUI_BUILD_DIR}}/scheduler_gui"
    echo ""

# Run the scheduler GUI
run-gui: gui release
    echo "Starting Scheduler GUI..."
    echo "Working directory: $(pwd)"
    ./{{GTK_GUI_BUILD_DIR}}/scheduler_gui

# Install GTK3 dependencies
install-gtk-deps:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Installing GTK+3.0 development libraries..."

    if command -v nix-env > /dev/null || [ -n "${NIX_PATH:-}" ]; then
        echo "NixOS detected. Installing via nix-env..."
        nix-env -iA nixos.gtk3 nixos.pkg-config
        echo "Or add to your configuration.nix:"
        echo "  environment.systemPackages = with pkgs; [ gtk3 pkg-config ];"
    elif command -v pacman > /dev/null; then
        echo "Arch Linux detected. Installing via pacman..."
        sudo pacman -S --needed gtk3 pkgconf
    elif command -v apt-get > /dev/null; then
        echo "Debian/Ubuntu detected. Installing via apt..."
        sudo apt-get update
        sudo apt-get install -y libgtk-3-dev pkg-config
    elif command -v dnf > /dev/null; then
        echo "Fedora/RHEL detected. Installing via dnf..."
        sudo dnf install -y gtk3-devel pkgconfig
    else
        echo ""
        echo "Could not detect package manager."
        echo "Please install GTK+3.0 development libraries manually:"
        echo "  - Arch/Manjaro: sudo pacman -S gtk3 pkgconf"
        echo "  - NixOS: nix-env -iA nixos.gtk3 nixos.pkg-config"
        echo "  - Debian/Ubuntu: sudo apt-get install libgtk-3-dev pkg-config"
        echo ""
        exit 1
    fi

    echo ""
    echo "GTK+3.0 installed successfully!"
    echo "Now run: just gui"


# ==================== TEST TARGETS ====================

# Build unit tests (requires Criterion)
tests:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p {{TESTING_DIR}}

    {{CC}} {{CFLAGS_CRITERION}} {{TEST_DIR}}/test_clk.c -o {{TESTING_DIR}}/test_clock {{LDFLAGS_CRITERION}}
    {{CC}} {{CFLAGS_CRITERION}} {{TEST_DIR}}/test_process.c -o {{TESTING_DIR}}/test_process {{LDFLAGS_CRITERION}}
    {{CC}} {{CFLAGS_CRITERION}} {{TEST_DIR}}/test_scheduler.c -o {{TESTING_DIR}}/test_scheduler {{LDFLAGS_CRITERION}}


# ==================== GTK4 TARGETS ====================

# Build dark GTK4 application
gtk:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p {{GTK_GUI_BUILD_OBJECT_DIR}} {{GTK_GUI_BUILD_DIR}}

    {{CC}} {{CFLAGS_GTK4}} -c {{GTK_GUI_SRC_DIR}}/dark.c -o {{GTK_GUI_BUILD_OBJECT_DIR}}/dark.o
    {{CC}} {{GTK_GUI_BUILD_OBJECT_DIR}}/dark.o -o {{GTK_GUI_BUILD_DIR}}/dark {{LDFLAGS_GTK4}}


# ==================== CLEAN TARGETS ====================

# Remove all build artifacts
clean: clean-scheduler clean-tests clean-gui

# Remove only scheduler build artifacts
clean-scheduler:
    rm -rf {{RELEASE_DIR}} {{DEBUG_DIR}}
    rm -f processes.txt scheduler.log scheduler.perf
    rm -f .osclock_key .osclock_marker .gen_input .pg_input

# Remove only test/gtk build artifacts
clean-tests:
    rm -rf {{TESTING_DIR}}
    rm -f *.o unit_tests

# Remove only GUI build artifacts
clean-gui:
    rm -rf {{GTK_GUI_BUILD_DIR}}
    rm -f scheduler_gui