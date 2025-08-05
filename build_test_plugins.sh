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
echo "🔁 Rebuilding and testing all plugins (basic + advanced)..."
echo "----------------------------------------------------------"

PASS=0
FAIL=0

mkdir -p output

for plugin in "${PLUGIN_TESTS[@]}"; do
  echo ""
  echo "🔨 Building basic test for plugin: $plugin"
  gcc -DPLUGIN=$plugin \
      -o output/test_${plugin}_basic \
      tests/test_all_plugins.c \
      plugins/$plugin.c \
      plugins/plugin_common.c \
      plugins/sync/monitor.c \
      plugins/sync/consumer_producer.c \
      -Iplugins -lpthread

  if [[ $? -ne 0 ]]; then
    echo "❌ Basic build failed for $plugin"
    ((FAIL++))
  else
    echo "▶️ Running basic test: ./output/test_${plugin}_basic"
    ./output/test_${plugin}_basic
    if [[ $? -eq 0 ]]; then
      echo "✅ Basic test PASSED for $plugin"
      ((PASS++))
    else
      echo "❌ Basic test FAILED for $plugin"
      ((FAIL++))
    fi
  fi

  echo ""
  echo "🔨 Building advanced test for plugin: $plugin"
  gcc -DPLUGIN=$plugin \
      -o output/test_${plugin}_advanced \
      tests/test_all_plugins.c \
      plugins/$plugin.c \
      plugins/plugin_common.c \
      plugins/sync/monitor.c \
      plugins/sync/consumer_producer.c \
      -Iplugins -lpthread

  if [[ $? -ne 0 ]]; then
    echo "❌ Advanced build failed for $plugin"
    ((FAIL++))
  else
    echo "▶️ Running advanced test: ./output/test_${plugin}_advanced"
    ./output/test_${plugin}_advanced
    if [[ $? -eq 0 ]]; then
      echo "✅ Advanced test PASSED for $plugin"
      ((PASS++))
    else
      echo "❌ Advanced test FAILED for $plugin"
      ((FAIL++))
    fi
  fi

  echo "-----------------------------"
done

echo ""
echo "🎯 Final Summary:"
echo "✅ Passed: $PASS"
echo "❌ Failed: $FAIL"
echo "============================="
