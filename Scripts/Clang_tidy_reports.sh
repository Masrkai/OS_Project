#!/usr/bin/env bash

REPORT_DIR="Docs/Clang_Reports"
mkdir -p "$REPORT_DIR"

echo "Running clang-tidy on C files..."

# Process files in src/
for file in src/*.c; do
    if [ -f "$file" ]; then
        echo "Analyzing: $file"
        report_name=$(basename "$file")
        clang-tidy "$file" -checks=* -- -I. 2>&1 > "$REPORT_DIR/${report_name}.clang-report.txt"
    fi
done

# Process files in src/schedulers/ (note: corrected path — likely should be src/schedulers/, not schedulers/)
if [ -d "src/schedulers" ]; then
    for file in src/schedulers/*.c; do
        if [ -f "$file" ]; then
            echo "Analyzing: $file"
            report_name=$(basename "$file")
            clang-tidy "$file" -checks=* -- -I. 2>&1 > "$REPORT_DIR/${report_name}.clang-report.txt"
        fi
    done
fi

echo "Reports generated in $REPORT_DIR/"