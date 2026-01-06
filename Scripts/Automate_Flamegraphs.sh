#!/usr/bin/env bash


# 1. Capture the Absolute Project Root
# This allows the script to find the Flamegraphs folder even after we 'cd'
PROJECT_ROOT=$(pwd)
PERF_DATA_DIR="$PROJECT_ROOT/Flamegraphs/Excutables_perf"
SVG_DIR="$PROJECT_ROOT/Flamegraphs/SVGs"
BUILD_DIR="$PROJECT_ROOT/build/debug"
EXE="./process_generator.out"

# Ensure directories exist
mkdir -p "$PERF_DATA_DIR"
mkdir -p "$SVG_DIR"

echo "🚀 Starting Performance Profiling (Running from $BUILD_DIR)..."

# 2. Change Directory to the Executable's Location
cd "$BUILD_DIR" || { echo "❌ Error: Could not find $BUILD_DIR"; exit 1; }

# ---------------------------------------------------------
# 3. CPU Profiling (Function Execution)
# ---------------------------------------------------------
echo "📊 Recording CPU execution..."
# Pipping '1' for HPF algorithm selection
echo "1" | sudo perf record -F 99 -g -o "$PERF_DATA_DIR/cpu_perf.data" $EXE

echo "🔥 Generating CPU Flame Graph..."
sudo perf script -i "$PERF_DATA_DIR/cpu_perf.data" | \
    stackcollapse-perf.pl | \
    flamegraph.pl --width=1500 --fontsize=14 --title="CPU Flame Graph" > "$SVG_DIR/cpu_flamegraph.svg"

# ---------------------------------------------------------
# 4. Memory Profiling (Page Faults & Memory Events)
# ---------------------------------------------------------
echo "🧠 Recording Memory (Page Faults)..."
# Using -e faults captures the stack trace every time the OS handles a page fault
echo "1" | sudo perf record -e faults -g -o "$PERF_DATA_DIR/mem_perf.data" $EXE

echo "🔥 Generating Memory Flame Graph..."
sudo perf script -i "$PERF_DATA_DIR/mem_perf.data" | \
    stackcollapse-perf.pl | \
    flamegraph.pl --color=mem --width=1500  --fontsize=14 --title="Memory Flame Graph (Page Faults)" > "$SVG_DIR/mem_flamegraph.svg"


# Return to root
cd "$PROJECT_ROOT"

echo "-------------------------------------------------------"
echo "✅ Analysis Complete!"
echo "📁 Data: $PERF_DATA_DIR"
echo "🖼️  SVGs: $SVG_DIR"