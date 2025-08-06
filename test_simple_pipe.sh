#!/bin/bash

# Define helper functions
print_status() {
    echo "✓ $1"
}

print_error() {
    echo "✗ $1" >&2
}

# Main test function
run_test() {
    local test_name="$1"
    local expected="$2"
    local input="$3"
    local plugins="$4"
    
    echo "=== Running Test: $test_name ==="
    
    # Create temporary input file
    local temp_input="/tmp/test_input_$$.txt"
    echo -e "$input\n<END>" > "$temp_input"
    
    echo "DEBUG: Input file contents:"
    cat "$temp_input"
    echo "---"
    
    # Run the analyzer with timeout - capture stdout only for actual output
    echo "DEBUG: Running analyzer..."
    local stdout_output=$(timeout 10 ./output/analyzer 10 $plugins < "$temp_input" 2>/dev/null)
    local stderr_output=$(timeout 10 ./output/analyzer 10 $plugins < "$temp_input" 2>&1 >/dev/null)
    
    echo "DEBUG: STDOUT output:"
    echo "$stdout_output"
    echo "DEBUG: STDERR output:"
    echo "$stderr_output"
    echo "---"
    
    # Filter for logger output from stdout only
    local actual=$(echo "$stdout_output" | grep "\[logger\]" | head -1)
    
    echo "EXPECTED: '$expected'"
    echo "ACTUAL: '$actual'"
    
    # Clean up temp file
    rm -f "$temp_input"
    
    # Compare results
    if [ "$actual" == "$expected" ]; then
        print_status "$test_name: PASS"
        return 0
    else
        print_error "$test_name: FAIL (Expected '$expected', got '$actual')"
        return 1
    fi
}

# Check if analyzer exists
if [ ! -f "./output/analyzer" ]; then
    print_error "Analyzer executable not found at ./output/analyzer"
    exit 1
fi

if [ ! -x "./output/analyzer" ]; then
    print_error "Analyzer is not executable"
    exit 1
fi

# Check if plugin files exist
for plugin in uppercaser logger; do
    if [ ! -f "./output/${plugin}.so" ]; then
        print_error "Plugin ${plugin}.so not found in ./output/"
        exit 1
    fi
done

echo "=== Starting Pipeline Tests ==="
echo

# Test 1: Basic uppercaser + logger
run_test "uppercaser + logger" "[logger] HELLO" "hello" "uppercaser logger"
test1_result=$?

echo
echo "=== Test Results Summary ==="
if [ $test1_result -eq 0 ]; then
    print_status "All tests passed!"
    exit 0
else
    print_error "Some tests failed!"
    exit 1
fi