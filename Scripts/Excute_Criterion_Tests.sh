#!/usr/bin/env bash

# Configuration
EXECUTABLE_DIR="build/debug/"
EXECUTABLE_NAME="test_generator.out"

INPUT_DIR="/path/to/tests/directory"
INPUT_VALUE="4"

# Navigate to executable directory and run the executable with input
cd "$EXECUTABLE_DIR" || { echo "Error: Could not cd to $EXECUTABLE_DIR"; exit 1; }
echo "$INPUT_VALUE" | ./"$EXECUTABLE_NAME" || { echo "Error: Executable failed"; exit 1; }

# Run all test files in the input directory
# Adjust file pattern as needed (e.g., *.sh, *.py, test_*.sh, etc.)
for test_file in "$INPUT_DIR"/*; do
    if [[ -f "$test_file" ]] && [[ -x "$test_file" ]]; then
        echo "Running test: $test_file"
        "$test_file"
        # Optionally check exit status:
        # if [ $? -ne 0 ]; then echo "Test $test_file failed"; fi
    fi
done