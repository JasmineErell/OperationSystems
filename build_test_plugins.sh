#!/bin/bash

PLUGIN_TESTS=(
  "logger"
  "uppercaser"
  "rotator"
  "flipper"
  "expander"
  "typewriter"
)

echo ""
echo "🔍 Running all plugin tests..."
echo "-----------------------------"

PASS=0
FAIL=0

for plugin in "${PLUGIN_TESTS[@]}"; do
  binary="./output/test_${plugin}"

  if [[ ! -x "$binary" ]]; then
    echo "⚠️  Skipping $plugin (binary missing)"
    continue
  fi

  echo "▶️ Running: $binary"
  output=$($binary)
  status=$?

  echo "$output"
  echo ""

  if [ "$status" -eq 0 ]; then
    echo "✅ $plugin test PASSED"
    ((PASS++))
  else
    echo "❌ $plugin test FAILED"
    ((FAIL++))
  fi

  echo "-----------------------------"
done

echo ""
echo "🎯 Final Summary:"
echo "✅ Passed: $PASS"
echo "❌ Failed: $FAIL"
echo "============================="
