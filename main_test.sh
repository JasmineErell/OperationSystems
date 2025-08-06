#!/bin/bash

# Exit on any error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Function to print colored output
print_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

print_category() {
    echo -e "${MAGENTA}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║${NC} $1"
    echo -e "${MAGENTA}╚══════════════════════════════════════════════════════════════╝${NC}"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_debug() {
    echo -e "${CYAN}[DEBUG]${NC} $1"
}

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

# Enhanced test function with detailed input/output display
run_test() {
    local test_name="$1"
    local expected_exit_code="$2"
    local command="$3"
    local expected_output="$4"
    local test_input="$5"
    local timeout_duration="${6:-10}"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${CYAN}─────────────────────────────────────────────────────${NC}"
    print_test "Running: $test_name"
    
    # Show test details upfront
    echo -e "${CYAN}📋 Test Configuration:${NC}"
    echo -e "${CYAN}   Command:${NC} $command"
    if [ -n "$test_input" ]; then
        echo -e "${CYAN}   Input:${NC} $(echo -e "$test_input" | tr '\n' ' → ' | sed 's/ → $//')"
    else
        echo -e "${CYAN}   Input:${NC} (no input)"
    fi
    if [ -n "$expected_output" ]; then
        echo -e "${CYAN}   Expected Pattern:${NC} $expected_output"
    fi
    echo -e "${CYAN}   Expected Exit Code:${NC} $expected_exit_code"
    echo ""
    
    # Create temporary files for output
    local stdout_file=$(mktemp)
    local stderr_file=$(mktemp)
    
    # Run the command with timeout
    local actual_exit_code=0
    local timed_out=0
    
    if [ -n "$test_input" ]; then
        if timeout "$timeout_duration" bash -c "echo -e '$test_input' | $command" > "$stdout_file" 2> "$stderr_file"; then
            actual_exit_code=0
        else
            actual_exit_code=$?
            if [ $actual_exit_code -eq 124 ]; then
                timed_out=1
                actual_exit_code=1
            fi
        fi
    else
        if timeout "$timeout_duration" bash -c "$command" > "$stdout_file" 2> "$stderr_file"; then
            actual_exit_code=0
        else
            actual_exit_code=$?
            if [ $actual_exit_code -eq 124 ]; then
                timed_out=1
                actual_exit_code=1
            fi
        fi
    fi
    
    local actual_stdout=$(cat "$stdout_file" 2>/dev/null || echo "")
    local actual_stderr=$(cat "$stderr_file" 2>/dev/null || echo "")
    
    # Always show the actual output
    echo -e "${CYAN}📤 Actual Results:${NC}"
    if [ -n "$actual_stdout" ]; then
        echo -e "${GREEN}   STDOUT:${NC}"
        echo "$actual_stdout" | sed 's/^/      /'
    else
        echo -e "${GREEN}   STDOUT:${NC} (empty)"
    fi
    
    if [ -n "$actual_stderr" ]; then
        echo -e "${RED}   STDERR:${NC}"
        echo "$actual_stderr" | sed 's/^/      /' | head -5
        # Warn about debug output
        if echo "$actual_stderr" | grep -qE "\[INFO\]|\[DEBUG\]"; then
            echo -e "${YELLOW}   ⚠️  Debug output detected - should be removed for submission${NC}"
        fi
    else
        echo -e "${RED}   STDERR:${NC} (empty)"
    fi
    
    echo -e "${CYAN}   Exit Code:${NC} $actual_exit_code"
    echo ""
    
    # Check if test passed
    local test_passed=0
    
    if [ $timed_out -eq 1 ]; then
        print_fail "$test_name - TIMEOUT after ${timeout_duration}s"
        echo -e "${YELLOW}💡 Program likely hung - check for deadlocks${NC}"
    elif [ "$actual_exit_code" -eq "$expected_exit_code" ]; then
        # Check output if provided
        if [ -n "$expected_output" ]; then
            if echo "$actual_stdout" | grep -q "$expected_output"; then
                print_pass "$test_name ✅"
                TESTS_PASSED=$((TESTS_PASSED + 1))
                test_passed=1
            else
                print_fail "$test_name - Expected output pattern not found ❌"
                echo -e "${YELLOW}   Looking for pattern: $expected_output${NC}"
                test_passed=0
            fi
        else
            print_pass "$test_name ✅"
            TESTS_PASSED=$((TESTS_PASSED + 1))
            test_passed=1
        fi
    else
        print_fail "$test_name - Exit code mismatch ❌"
        echo -e "${YELLOW}   Expected: $expected_exit_code, Got: $actual_exit_code${NC}"
        test_passed=0
    fi
    
    # Cleanup temporary files
    rm -f "$stdout_file" "$stderr_file"
    echo ""
}

# Function to check memory leaks with valgrind (if available)
check_memory_leaks() {
    local test_name="$1"
    local command="$2"
    local test_input="$3"
    
    if ! command -v valgrind &> /dev/null; then
        print_warning "Valgrind not found, skipping memory leak test for: $test_name"
        return
    fi
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${CYAN}─────────────────────────────────────────────────────${NC}"
    print_test "Memory Check: $test_name"
    
    echo -e "${CYAN}📋 Test Configuration:${NC}"
    echo -e "${CYAN}   Command:${NC} valgrind --leak-check=full $command"
    if [ -n "$test_input" ]; then
        echo -e "${CYAN}   Input:${NC} $(echo -e "$test_input" | tr '\n' ' → ' | sed 's/ → $//')"
    fi
    echo ""
    
    local valgrind_output=$(mktemp)
    local stdout_file=$(mktemp)
    
    # Run with valgrind
    if [ -n "$test_input" ]; then
        echo -e "$test_input" | valgrind --leak-check=full --error-exitcode=1 --log-file="$valgrind_output" \
            eval "$command" > "$stdout_file" 2>/dev/null || true
    else
        valgrind --leak-check=full --error-exitcode=1 --log-file="$valgrind_output" \
            eval "$command" > "$stdout_file" 2>/dev/null || true
    fi
    
    echo -e "${CYAN}📤 Memory Analysis:${NC}"
    
    # Check for memory leaks
    if grep -q "All heap blocks were freed -- no leaks are possible" "$valgrind_output" ||
       grep -q "definitely lost: 0 bytes in 0 blocks" "$valgrind_output"; then
        echo -e "${GREEN}   Memory Status: No leaks detected ✅${NC}"
        print_pass "Memory check: $test_name ✅"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}   Memory Status: Leaks detected ❌${NC}"
        print_fail "Memory leak detected in: $test_name ❌"
        echo -e "${YELLOW}   Valgrind summary:${NC}"
        grep -E "(lost:|ERROR SUMMARY)" "$valgrind_output" | head -3 | sed 's/^/      /'
    fi
    
    rm -f "$valgrind_output" "$stdout_file"
    echo ""
}

# Build the project first
print_test "Building project..."
if ./build.sh; then
    print_pass "Build successful"
else
    print_fail "Build failed - cannot continue with tests"
    exit 1
fi
echo ""

# Check if output directory and files exist
if [ ! -d "output" ]; then
    print_fail "Output directory not found"
    exit 1
fi

if [ ! -f "output/analyzer" ]; then
    print_fail "Main executable not found"
    exit 1
fi

# Make sure analyzer is executable
chmod +x output/analyzer

echo ""
echo -e "${BLUE}🧪 COMPREHENSIVE MODULAR PIPELINE TEST SUITE${NC}"
echo -e "${BLUE}==============================================${NC}"
echo ""

# ===================================================================
# CATEGORY 1: VALID SIMPLE RUNS (at most 2 plugins)
# ===================================================================
print_category "CATEGORY 1: Valid Simple Runs (≤2 plugins)"
echo ""

# Single plugin tests
run_test "1.1 Single plugin - logger only" 0 "./output/analyzer 10 logger" "\\[logger\\] hello" "hello\n<END>" 5

run_test "1.2 Single plugin - uppercaser (no logger)" 0 "./output/analyzer 5 uppercaser" "HELLO" "hello\n<END>" 5

run_test "1.3 Single plugin - flipper (no logger)" 0 "./output/analyzer 8 flipper" "olleh" "hello\n<END>" 5

# Two plugin combinations
run_test "1.4 Two plugins - uppercaser → logger" 0 "./output/analyzer 15 uppercaser logger" "\\[logger\\] HELLO" "hello\n<END>" 5

run_test "1.5 Two plugins - rotator → logger" 0 "./output/analyzer 12 rotator logger" "\\[logger\\] ohell" "hello\n<END>" 5

run_test "1.6 Two plugins - flipper → logger" 0 "./output/analyzer 6 flipper logger" "\\[logger\\] olleh" "hello\n<END>" 5

run_test "1.7 Two plugins - expander → logger" 0 "./output/analyzer 20 expander logger" "\\[logger\\] h e l l o" "hello\n<END>" 5

run_test "1.8 Two plugins - uppercaser → flipper (no logger)" 0 "./output/analyzer 3 uppercaser flipper" "OLLEH" "hello\n<END>" 5

# ===================================================================
# CATEGORY 2: VALID ADVANCED RUNS (repeated plugins, complex chains)
# ===================================================================
echo ""
print_category "CATEGORY 2: Valid Advanced Runs (complex chains)"
echo ""

# Multiple same plugins
run_test "2.1 Double rotator - rotator → rotator → logger" 0 "./output/analyzer 5 rotator rotator logger" "\\[logger\\] lohel" "hello\n<END>" 8

run_test "2.2 Triple rotator - rotator³ → logger" 0 "./output/analyzer 7 rotator rotator rotator logger" "\\[logger\\] llohe" "hello\n<END>" 8

run_test "2.3 Double flipper - flipper → flipper → logger" 0 "./output/analyzer 4 flipper flipper logger" "\\[logger\\] hello" "hello\n<END>" 8

run_test "2.4 Double logger - uppercaser → logger → logger" 0 "./output/analyzer 10 uppercaser logger logger" "\\[logger\\] HELLO" "hello\n<END>" 8

# Complex mixed chains
run_test "2.5 Mixed chain - uppercaser → rotator → flipper → logger" 0 "./output/analyzer 12 uppercaser rotator flipper logger" "\\[logger\\] LLOHE" "hello\n<END>" 10

run_test "2.6 Long chain - uppercaser → rotator → rotator → flipper → expander → logger" 0 "./output/analyzer 15 uppercaser rotator rotator flipper expander logger" "\\[logger\\]" "test\n<END>" 10

run_test "2.7 Complex repeats - rotator → flipper → rotator → uppercaser → logger" 0 "./output/analyzer 8 rotator flipper rotator uppercaser logger" "\\[logger\\]" "abc\n<END>" 10

# Multiple inputs with complex chain
run_test "2.8 Multiple inputs + complex chain" 0 "./output/analyzer 20 uppercaser rotator logger" "\\[logger\\]" "hello1\nhello2\nworld\n<END>" 10

# Large queue with simple chain
run_test "2.9 Large queue test" 0 "./output/analyzer 100 uppercaser flipper logger" "\\[logger\\] DLROW" "world\n<END>" 8

# ===================================================================
# CATEGORY 3: INVALID INPUT TESTS
# ===================================================================
echo ""
print_category "CATEGORY 3: Invalid Input Tests"
echo ""

run_test "3.1 No arguments" 1 "./output/analyzer" "Usage:" "" 3

run_test "3.2 Missing plugins" 1 "./output/analyzer 10" "Usage:" "" 3

run_test "3.3 Invalid queue size - non-numeric" 1 "./output/analyzer abc logger" "Usage:" "" 3

run_test "3.4 Invalid queue size - negative" 1 "./output/analyzer -5 logger" "Usage:" "" 3

run_test "3.5 Invalid queue size - zero" 1 "./output/analyzer 0 logger" "Usage:" "" 3

run_test "3.6 Invalid queue size - leading zero" 1 "./output/analyzer 01 logger" "Usage:" "" 3

run_test "3.7 Non-existent plugin" 1 "./output/analyzer 10 nonexistent" "" "" 3

run_test "3.8 Mix of valid and invalid plugins" 1 "./output/analyzer 10 logger badplugin uppercaser" "" "" 3

run_test "3.9 Invalid queue size - decimal" 1 "./output/analyzer 10.5 logger" "Usage:" "" 3

# ===================================================================
# CATEGORY 4: INITIALIZATION AND GRACEFUL SHUTDOWN
# ===================================================================
echo ""
print_category "CATEGORY 4: Initialization and Graceful Shutdown"
echo ""

run_test "4.1 Empty input - immediate END" 0 "./output/analyzer 10 logger" "Pipeline shutdown complete" "<END>" 5

run_test "4.2 Graceful shutdown with multiple plugins" 0 "./output/analyzer 8 uppercaser rotator flipper logger" "Pipeline shutdown complete" "test\n<END>" 8

run_test "4.3 Small queue size - tests blocking" 0 "./output/analyzer 2 logger" "Pipeline shutdown complete" "a\nb\nc\n<END>" 8

run_test "4.4 Single character input" 0 "./output/analyzer 5 logger" "\\[logger\\] x" "x\n<END>" 5

run_test "4.5 Multiple lines before END" 0 "./output/analyzer 10 logger" "\\[logger\\]" "line1\nline2\nline3\n<END>" 8

run_test "4.6 Long input line (within limits)" 0 "./output/analyzer 15 logger" "\\[logger\\]" "$(printf 'a%.0s' {1..100})\n<END>" 8

run_test "4.7 Stress test - many short inputs" 0 "./output/analyzer 25 logger" "Pipeline shutdown complete" "$(for i in {1..10}; do echo "test$i"; done; echo '<END>')" 15

run_test "4.8 Typewriter plugin - timing test" 0 "./output/analyzer 5 typewriter" "" "quick\n<END>" 15

# ===================================================================
# CATEGORY 5: MEMORY ALLOCATION AND MANAGEMENT
# ===================================================================
echo ""
print_category "CATEGORY 5: Memory Allocation and Management"
echo ""

# Test memory with different scenarios
check_memory_leaks "5.1 Simple memory test" "./output/analyzer 10 logger" "test\n<END>"

check_memory_leaks "5.2 Complex chain memory test" "./output/analyzer 5 uppercaser rotator flipper logger" "hello\nworld\n<END>"

check_memory_leaks "5.3 Repeated plugins memory test" "./output/analyzer 3 rotator rotator rotator logger" "test\n<END>"

check_memory_leaks "5.4 Large input memory test" "./output/analyzer 20 expander logger" "$(printf 'x%.0s' {1..30})\n<END>"

check_memory_leaks "5.5 Multiple inputs memory test" "./output/analyzer 15 uppercaser logger" "$(for i in {1..5}; do echo "line$i"; done; echo '<END>')"

# Memory stress test without valgrind
TESTS_RUN=$((TESTS_RUN + 1))
echo -e "${CYAN}─────────────────────────────────────────────────────${NC}"
print_test "5.6 Memory stress test - rapid allocation/deallocation"

# Create large input for stress testing
STRESS_INPUT=""
for i in {1..20}; do
    STRESS_INPUT="${STRESS_INPUT}line${i}\n"
done
STRESS_INPUT="${STRESS_INPUT}<END>"

echo -e "${CYAN}📋 Test Configuration:${NC}"
echo -e "${CYAN}   Command:${NC} ./output/analyzer 10 uppercaser flipper expander logger"
echo -e "${CYAN}   Input:${NC} 20 lines of text + <END>"
echo -e "${CYAN}   Purpose:${NC} Stress test memory allocation/deallocation"
echo ""

start_time=$(date +%s%N)
echo -e "$STRESS_INPUT" | ./output/analyzer 10 uppercaser flipper expander logger > /dev/null 2>&1
stress_exit_code=$?
end_time=$(date +%s%N)
duration_ms=$(( (end_time - start_time) / 1000000 ))

echo -e "${CYAN}📤 Performance Results:${NC}"
echo -e "${CYAN}   Exit Code:${NC} $stress_exit_code"
echo -e "${CYAN}   Duration:${NC} ${duration_ms}ms"
echo ""

if [ $stress_exit_code -eq 0 ] && [ $duration_ms -lt 3000 ]; then
    print_pass "5.6 Memory stress test ✅"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_fail "5.6 Memory stress test ❌"
    if [ $stress_exit_code -ne 0 ]; then
        echo -e "${YELLOW}   Exit code: $stress_exit_code${NC}"
    fi
    if [ $duration_ms -ge 3000 ]; then
        echo -e "${YELLOW}   Too slow: ${duration_ms}ms (expected <3000ms)${NC}"
    fi
fi
echo ""

# ===================================================================
# FINAL SUMMARY
# ===================================================================
echo ""
echo -e "${BLUE}🏁 COMPREHENSIVE TEST RESULTS${NC}"
echo -e "${BLUE}==============================${NC}"
echo -e "${BLUE}Tests run: $TESTS_RUN${NC}"
echo -e "${GREEN}Tests passed: $TESTS_PASSED${NC}"
echo -e "${RED}Tests failed: $((TESTS_RUN - TESTS_PASSED))${NC}"
echo ""

if [ $TESTS_PASSED -eq $TESTS_RUN ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED! Your pipeline system is working correctly.${NC}"
    echo ""
    echo -e "${YELLOW}⚠️  REMINDER: Remove debug output from STDERR before submission:${NC}"
    echo -e "${YELLOW}   - Remove [INFO] logs${NC}"
    echo -e "${YELLOW}   - Remove [DEBUG] logs${NC}"
    echo -e "${YELLOW}   - Keep only plugin output and error messages${NC}"
    exit 0
else
    echo -e "${RED}❌ SOME TESTS FAILED! Review the failed tests above.${NC}"
    echo ""
    echo -e "${YELLOW}Common issues to check:${NC}"
    echo -e "${YELLOW}   - Memory management (double free, leaks)${NC}"
    echo -e "${YELLOW}   - Plugin transformation logic${NC}"
    echo -e "${YELLOW}   - Thread synchronization${NC}"
    echo -e "${YELLOW}   - Error handling${NC}"
    exit 1
fi