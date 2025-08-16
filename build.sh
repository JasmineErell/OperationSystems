#!/bin/bash

# Exit on any error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Create output directory if it doesn't exist
print_status "Creating output directory..."
mkdir -p output

# Build main application
print_status "Building main application..."
gcc -o analyzer main.c main.h -ldl -lpthread || {
    print_error "Failed to build main application"
    exit 1
}

# Build plugins
plugin_list="logger uppercaser rotator flipper expander typewriter"

for plugin_name in $plugin_list; do
    print_status "Building plugin: $plugin_name"
    
    # Check if plugin source file exists
    if [ ! -f "plugins/${plugin_name}.c" ]; then
        print_warning "Plugin source file plugins/${plugin_name}.c not found, skipping..."
        continue
    fi
    
    gcc -fPIC -shared -o output/${plugin_name}.so \
        plugins/${plugin_name}.c \
        plugins/plugin_common.c \
        plugins/sync/monitor.c \
        plugins/sync/consumer_producer.c \
        -ldl -lpthread || {
        print_error "Failed to build plugin: $plugin_name"
        exit 1
    }
done

print_status "Build completed successfully!"
print_status "Built files:"
ls -la output/