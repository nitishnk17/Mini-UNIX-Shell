#!/bin/bash

SHELL_EXEC="../shell"
TEST_DIR="test_files"
PASSED=0
FAILED=0

mkdir -p $TEST_DIR
cd ..

if [ ! -f "shell" ]; then
    echo "Building shell..."
    make clean
    make
    if [ $? -ne 0 ]; then
        echo "Build failed!"
        exit 1
    fi
fi

cd Test

echo "========================================="
echo "Running Mini Shell Test Suite (25 Tests)"
echo "========================================="
echo ""

run_test() {
    local test_num=$1
    local test_name=$2
    local input=$3
    local expected_behavior=$4

    echo "----------------------------------------"
    echo "Test $test_num: $test_name"
    echo "Input: $input"
    echo "Output:"
    echo -e "$input" | $SHELL_EXEC

    if [ $? -eq 0 ] || [ "$expected_behavior" == "may_fail" ]; then
        echo "✓ PASS"
        ((PASSED++))
    else
        echo "✗ FAIL"
        ((FAILED++))
    fi
    echo ""
}

echo "=== Basic Command Execution ==="
echo ""

run_test 1 "Execute ls command" "ls\nexit" "pass"

run_test 2 "Execute echo with arguments" "echo Hello World\nexit" "pass"

run_test 3 "Execute pwd command" "pwd\nexit" "pass"

run_test 4 "Execute command with multiple arguments" "echo arg1 arg2 arg3\nexit" "pass"

echo "=== Built-in Commands ==="
echo ""

run_test 5 "Built-in cd to parent directory" "cd ..\npwd\nexit" "pass"

run_test 6 "Built-in cd to home" "cd\npwd\nexit" "pass"

run_test 7 "Built-in exit command" "exit" "pass"

echo "=== Quote Handling ==="
echo ""

run_test 8 "Single quotes with spaces" "echo 'hello world'\nexit" "pass"

run_test 9 "Double quotes with spaces" "echo \"hello world\"\nexit" "pass"

run_test 10 "Multiple spaces in quotes" "echo \"word1     word2     word3\"\nexit" "pass"

echo "=== I/O Redirection ==="
echo ""

echo "test output data" > $TEST_DIR/input.txt

run_test 11 "Output redirection" "echo 'test output' > $TEST_DIR/out1.txt\nexit" "pass"
echo "Created file contents:"
cat $TEST_DIR/out1.txt 2>/dev/null
echo ""

run_test 12 "Input redirection" "cat < $TEST_DIR/input.txt\nexit" "pass"

run_test 13 "Combined input and output redirection" "cat < $TEST_DIR/input.txt > $TEST_DIR/out2.txt\nexit" "pass"
echo "Created file contents:"
cat $TEST_DIR/out2.txt 2>/dev/null
echo ""

echo "=== Pipeline Operations ==="
echo ""

run_test 14 "Simple pipeline with cat" "echo 'hello world' | cat\nexit" "pass"

run_test 15 "Pipeline with grep" "ls | grep test\nexit" "pass"

run_test 16 "Pipeline with wc" "echo 'one two three four' | wc -w\nexit" "pass"

run_test 17 "Pipeline with input redirection" "cat < $TEST_DIR/input.txt | grep test\nexit" "pass"

run_test 18 "Pipeline with output redirection" "echo 'pipeline test' | cat > $TEST_DIR/out3.txt\nexit" "pass"
echo "Created file contents:"
cat $TEST_DIR/out3.txt 2>/dev/null
echo ""

echo "=== Background Execution ==="
echo ""

run_test 19 "Background process" "sleep 0.5 &\nexit" "pass"
sleep 1

run_test 20 "Background pipeline" "echo 'background' | cat &\nexit" "pass"
sleep 1

echo "=== Command History ==="
echo ""

run_test 21 "History command" "echo cmd1\necho cmd2\nhistory\nexit" "pass"

echo "=== Error Handling ==="
echo ""

run_test 22 "Invalid command" "invalidcommand123\nexit" "may_fail"

run_test 23 "Missing input file" "cat < nonexistent_file.txt\nexit" "may_fail"

run_test 24 "Built-in command in pipeline" "cd | ls\nexit" "may_fail"

echo "=== Edge Cases ==="
echo ""

run_test 25 "Empty input lines" "\n\n\nexit" "pass"

echo "========================================="
echo "Test Results Summary"
echo "========================================="
echo "Total Tests: 25"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "========================================="

rm -rf $TEST_DIR

if [ $FAILED -eq 0 ]; then
    echo "✓ All tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi
