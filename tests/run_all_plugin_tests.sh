#!/bin/bash

set -e
set -u
 
PLUGINS=( "flipper" "rotator" "uppercaser" "expander" "logger" "typewriter") 
TEST_FILE="test_all_plugins_comprehensive.c"
OUTPUT_DIR="../output"  # because you're inside tests/

mkdir -p "$OUTPUT_DIR"

echo "=============================="
echo "🔁 Running Plugin Tests"
echo "=============================="

for PLUGIN in "${PLUGINS[@]}"; do
    echo "🧪 Testing plugin: $PLUGIN"

    OUTPUT_BIN="${OUTPUT_DIR}/test_${PLUGIN}_comprehensive"

    gcc -std=c99 -Wall -Wextra -g -O2 \
        -DPLUGIN=$PLUGIN \
        -o "$OUTPUT_BIN" \
        "$TEST_FILE" \
        "../output/${PLUGIN}.so" \
        -ldl -lpthread -lrt

    echo "▶️ Running: $OUTPUT_BIN"
    "$OUTPUT_BIN"
    echo "✅ $PLUGIN test finished"
    echo "------------------------------"
done

echo "🎉 All plugin tests completed!"
