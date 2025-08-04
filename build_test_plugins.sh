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
echo "🔁 Rebuilding and testing all plugins..."
echo "----------------------------------------"

PASS=0
FAIL=0

mkdir -p output

for plugin in "${PLUGIN_TESTS[@]}"; do
  echo "🔨 Building test for plugin: $plugin"

  gcc -DPLUGIN=$plugin \
      -o output/test_$plugin \
      tests/test_all_plugins.c \
      plugins/$plugin.c \
      plugins/plugin_common.c \
      plugins/sync/monitor.c \
      plugins/sync/consumer_producer.c \
      -Iplugins -lpthread

  if [[ $? -ne 0 ]]; then
    echo "❌ Build failed for $plugin"
    ((FAIL++))
    echo "-----------------------------"
    continue
  fi

  echo "✅ Built: output/test_$plugin"

  echo "▶️ Running: ./output/test_$plugin"
  output=$(./output/test_$plugin)
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
