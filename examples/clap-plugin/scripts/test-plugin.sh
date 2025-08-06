#!/bin/bash

# CLAP Saw Demo Plugin Testing Script
# Builds the plugin and runs clap-validator to ensure CLAP specification compliance

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
VALIDATOR_DIR="$PROJECT_ROOT/libs/clap-validator"

echo -e "${BLUE}=== CLAP Saw Demo Plugin Testing ===${NC}"
echo "Project Root: $PROJECT_ROOT"
echo "Build Directory: $BUILD_DIR"
echo ""

# Function to print colored status messages
print_status() {
  echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
  echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
  echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
  echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Rust is installed (needed for clap-validator)
if ! command -v cargo &> /dev/null; then
  print_error "Rust/Cargo not found. Please install Rust from https://rustup.rs/"
  exit 1
fi

# Step 1: Build clap-validator if needed
print_status "Building clap-validator..."
cd "$VALIDATOR_DIR"
if ! cargo build --release; then
  print_error "Failed to build clap-validator"
  exit 1
fi
print_success "clap-validator built successfully"

# Step 2: Build the plugin
print_status "Building CLAP Saw Demo plugin..."
cd "$PROJECT_ROOT"

# Configure CMake if needed
if [ ! -d "$BUILD_DIR" ]; then
  print_status "Configuring CMake build..."
  cmake -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCOPY_AFTER_BUILD=ON
fi

# Build the plugin
if ! cmake --build "$BUILD_DIR" -j --target clap-saw-demo; then
  print_error "Failed to build plugin"
  exit 1
fi
print_success "Plugin built successfully"

# Step 3: Locate the built plugin
PLUGIN_PATH="$BUILD_DIR/clap-saw-demo.clap"
if [ ! -d "$PLUGIN_PATH" ]; then
  print_error "Plugin not found at: $PLUGIN_PATH"
  print_status "Available files in build directory:"
  ls -la "$BUILD_DIR" || true
  exit 1
fi

print_success "Plugin found at: $PLUGIN_PATH"

# Step 4: Run clap-validator
print_status "Running CLAP specification validation..."
echo ""
echo -e "${YELLOW}=== CLAP Validator Output ===${NC}"

cd "$VALIDATOR_DIR"
VALIDATOR_OUTPUT=$(cargo run --release -- validate "$PLUGIN_PATH" --only-failed 2>&1)
VALIDATOR_EXIT_CODE=$?

echo "$VALIDATOR_OUTPUT"

# Check for actual validation errors in output (not just warnings)
if echo "$VALIDATOR_OUTPUT" | grep -q "\[ERROR\]"; then
  VALIDATOR_RESULT=1
  print_error "CLAP validation found errors in plugin"
elif [ $VALIDATOR_EXIT_CODE -ne 0 ]; then
  VALIDATOR_RESULT=$VALIDATOR_EXIT_CODE
  print_error "CLAP validation failed with exit code: $VALIDATOR_EXIT_CODE"
else
  VALIDATOR_RESULT=0
  print_success "CLAP validation passed!"
fi

echo ""
echo -e "${YELLOW}=== Validation Summary ===${NC}"

if [ $VALIDATOR_RESULT -eq 0 ]; then
  print_success "✓ Plugin successfully passed all CLAP specification tests"
  print_status "Your plugin is ready for testing in DAWs!"
else
  print_error "✗ Plugin failed CLAP specification validation"
  print_status "Please fix the issues above before proceeding"
  
  echo ""
  print_status "For debugging a specific test, use:"
  echo "  cd $VALIDATOR_DIR"
  echo "  cargo run --release -- list tests"
  echo "  cargo run --release -- validate --in-process --test-filter <test-name> $PLUGIN_PATH"
fi

echo ""
print_status "Testing complete"
exit $VALIDATOR_RESULT 