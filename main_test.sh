#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_TOTAL=0

run_test() {
    local name="$1"
    local expected_exit="$2"
    local cmd="$3"
    local expected_out="$4"
    local input="$5"
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo "Test $TESTS_TOTAL: $name"
    
    local actual_out
    local actual_exit=0
    
    if [ -n "$input" ]; then
        actual_out=$(echo -e "$input" | $cmd 2>&1) || actual_exit=$?
    else
        actual_out=$($cmd 2>&1) || actual_exit=$?
    fi
    
    if [ "$actual_exit" -eq "$expected_exit" ]; then
        if [ -n "$expected_out" ]; then
            if echo "$actual_out" | grep -q "$expected_out"; then
                echo -e "${GREEN}PASS${NC}"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            else
                echo -e "${RED}FAIL${NC} - output mismatch"
                echo "Expected: $expected_out"
                echo "Got: $actual_out"
            fi
        else
            echo -e "${GREEN}PASS${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        fi
    else
        echo -e "${RED}FAIL${NC} - exit code mismatch"
        echo "Expected exit: $expected_exit, got: $actual_exit"
        echo "Output: $actual_out"
    fi
    echo ""
}

echo "Building project"
./build.sh
echo ""

echo "Running tests"
echo ""

# Valid tests
run_test "Single logger" 0 "./output/analyzer 10 logger" "\\[logger\\] hello" "hello\n<END>"
run_test "Uppercaser only" 0 "./output/analyzer 5 uppercaser" "Pipeline shutdown complete" "hello\n<END>"
run_test "Flipper only" 0 "./output/analyzer 8 flipper" "Pipeline shutdown complete" "hello\n<END>"
run_test "Uppercaser to logger" 0 "./output/analyzer 15 uppercaser logger" "\\[logger\\] HELLO" "hello\n<END>"
run_test "Rotator to logger" 0 "./output/analyzer 12 rotator logger" "\\[logger\\] ohell" "hello\n<END>"
run_test "Double rotator" 0 "./output/analyzer 5 rotator rotator logger" "\\[logger\\] lohel" "hello\n<END>"
run_test "Double flipper" 0 "./output/analyzer 4 flipper flipper logger" "\\[logger\\] hello" "hello\n<END>"
run_test "Complex chain" 0 "./output/analyzer 12 uppercaser rotator flipper logger" "\\[logger\\] LLEHO" "hello\n<END>"
run_test "Multiple inputs" 0 "./output/analyzer 20 uppercaser logger" "\\[logger\\] HELLO" "hello\nworld\ntest\n<END>"
run_test "Empty END" 0 "./output/analyzer 10 logger" "Pipeline shutdown complete" "<END>"
run_test "Small queue" 0 "./output/analyzer 2 logger" "Pipeline shutdown complete" "a\nb\nc\n<END>"

# Invalid tests
run_test "No arguments" 1 "./output/analyzer" "Usage:" ""
run_test "Missing plugins" 1 "./output/analyzer 10" "Usage:" ""
run_test "Zero queue size" 1 "./output/analyzer 0 logger" "Usage:" ""
run_test "Negative queue" 1 "./output/analyzer -5 logger" "Usage:" ""
run_test "Non-numeric queue" 1 "./output/analyzer abc logger" "Usage:" ""
run_test "Decimal queue" 1 "./output/analyzer 10.5 logger" "Usage:" ""
run_test "Leading zero" 1 "./output/analyzer 01 logger" "Usage:" ""
run_test "Bad plugin" 1 "./output/analyzer 10 nonexistent" "dlopen failed" ""

# Memory test
echo "Memory stress test"
STRESS_INPUT=""
for i in {1..50}; do
    STRESS_INPUT="${STRESS_INPUT}line${i}\n"
done
STRESS_INPUT="${STRESS_INPUT}<END>"

if echo -e "$STRESS_INPUT" | timeout 30 ./output/analyzer 5 uppercaser rotator flipper logger > /dev/null 2>&1; then
    echo -e "${GREEN}PASS${NC} - Memory stress test"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC} - Memory stress test"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

echo ""
echo "Results: $TESTS_PASSED/$TESTS_TOTAL tests passed"

if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi