# #!/bin/bash

# # Exit on any error
# set -e

# # Colors for output
# RED='\033[0;31m'
# GREEN='\033[0;32m'
# YELLOW='\033[1;33m'
# NC='\033[0m' # No Color

# # Function to print colored output
# print_status() {
#     echo -e "${GREEN}[BUILD]${NC} $1"
# }

# print_warning() {
#     echo -e "${YELLOW}[WARNING]${NC} $1"
# }

# print_error() {
#     echo -e "${RED}[ERROR]${NC} $1"
# }

# # Create output directory if it doesn't exist
# print_status "Creating output directory..."
# mkdir -p output

# # Build main application
# print_status "Building main application..."
# gcc -o output/analyzer main.c -ldl -lpthread || 
# {
#     print_error "Failed to build main application"
#     exit 1
# }

# # Build plugins
# plugin_list="logger uppercaser rotator flipper expander typewriter"

# for plugin_name in $plugin_list; do
#     print_status "Building plugin: $plugin_name"
    
#     # Check if plugin source file exists
#     if [ ! -f "plugins/${plugin_name}.c" ]; then
#         print_warning "Plugin source file plugins/${plugin_name}.c not found, skipping..."
#         continue
#     fi
    
#     gcc -fPIC -shared -o output/${plugin_name}.so \
#         plugins/${plugin_name}.c \
#         plugins/plugin_common.c \
#         plugins/sync/monitor.c \
#         plugins/sync/consumer_producer.c \
#         -ldl -lpthread || {
#         print_error "Failed to build plugin: $plugin_name"
#         exit 1
#     }
# done

# print_status "Build completed successfully!"
# print_status "Built files:"
# ls -la output/



#!/bin/bash

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
print_status "Creating output directory"
mkdir -p output

# Build main application
print_status "Building main"
gcc -o output/analyzer main.c -ldl -lpthread || {
    print_error "Failed to build main application"
    exit 1
}

# Build plugins - discover them dynamically
print_status "Building plugins"

# Check if required infrastructure files exist first
if [ ! -f "plugins/plugin_common.c" ]; then
    print_error "plugins/plugin_common.c not found - required for all plugins"
    exit 1
fi

if [ ! -f "plugins/sync/monitor.c" ]; then
    print_error "plugins/sync/monitor.c not found - required for all plugins"
    exit 1
fi

if [ ! -f "plugins/sync/consumer_producer.c" ]; then
    print_error "plugins/sync/consumer_producer.c not found - required for all plugins"
    exit 1
fi

# Build plugins actually
plugin_count=0
for plugin_file in plugins/*.c; do
    # skip if glob didn't match any files
    if [ ! -f "$plugin_file" ]; then
        continue
    fi
    
    # extract pluggin name without path and extension
    plugin_name=$(basename "$plugin_file" .c)
    
    # skip plugin_common.c 
    if [ "$plugin_name" = "plugin_common" ]; then
        continue
    fi
    
    print_status "Building plugin: $plugin_name"
    
    gcc -fPIC -shared -o "output/${plugin_name}.so" \
        "$plugin_file" \
        plugins/plugin_common.c \
        plugins/sync/monitor.c \
        plugins/sync/consumer_producer.c \
        -ldl -lpthread || {
        print_error "Failed to build plugin: $plugin_name"
        exit 1
    }
    
    plugin_count=$((plugin_count + 1))
done

if [ $plugin_count -eq 0 ]; then
    print_warning "No plugins found in plugins/ directory"
else
    print_status "Successfully built $plugin_count plugin(s)"
fi


