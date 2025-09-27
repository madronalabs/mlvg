#!/bin/bash

# CLAP Saw Demo Plugin Debugging Script
# Comprehensive plugin testing using the full CLAP debugging toolkit

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
TOOLS_DIR="$PROJECT_ROOT/tools"

echo -e "${CYAN}=== CLAP Saw Demo Plugin Debugging Suite ===${NC}"
echo "Project Root: $PROJECT_ROOT"
echo "Tools Directory: $TOOLS_DIR"
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

print_section() {
  echo ""
  echo -e "${CYAN}=== $1 ===${NC}"
}

# Check dependencies
check_dependencies() {
  print_section "Checking Dependencies"
  
  local missing_deps=0
  
  # Check if tools are available
  if [ ! -d "$TOOLS_DIR/clap-info" ]; then
    print_error "clap-info not found. Run: git submodule update --init --recursive"
    missing_deps=1
  fi
  
  if [ ! -d "$TOOLS_DIR/clap-host" ]; then
    print_error "clap-host not found. Run: git submodule update --init --recursive"  
    missing_deps=1
  fi
  
  # Check for build tools
  if ! command -v cmake &> /dev/null; then
    print_error "CMake not found"
    missing_deps=1
  fi
  
  if ! command -v cargo &> /dev/null; then
    print_error "Rust/Cargo not found (needed for clap-validator)"
    missing_deps=1
  fi
  
  # Check for Qt6 (needed for clap-host)
  if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
    print_warning "Qt6 not found - clap-host will not be available"
    print_status "Install Qt6: brew install qt6 (macOS) or apt install qt6-base-dev (Linux)"
  fi
  
  if [ $missing_deps -eq 1 ]; then
    print_error "Missing required dependencies. Please install them first."
    exit 1
  fi
  
  print_success "All required dependencies found"
}

# Build all debugging tools
build_tools() {
  print_section "Building Debugging Tools"
  
  # Build clap-info
  print_status "Building clap-info..."
  cd "$TOOLS_DIR/clap-info"
  if [ ! -d "build" ]; then
    cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
  fi
  cmake --build build --config Release
  print_success "clap-info built successfully"
  
  # Build clap-host (if Qt and audio dependencies are available)
  print_status "Building clap-host..."
  cd "$TOOLS_DIR/clap-host"
  if command -v qmake6 &> /dev/null || command -v qmake &> /dev/null; then
    # Check for required audio dependencies on macOS
    if [[ "$OSTYPE" == "darwin"* ]]; then
      if ! brew list rtmidi &> /dev/null || ! brew list rtaudio &> /dev/null; then
        print_warning "Installing required audio dependencies..."
        brew install rtmidi rtaudio
      fi
    fi
    
    # Clean and rebuild if needed
    if [ ! -f "build/host/clap-host" ]; then
      rm -rf build builds
      cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
      cmake --build build --config Release
    fi
    
    if [ -f "build/host/clap-host" ]; then
      print_success "clap-host built successfully"
    else
      print_error "clap-host build failed"
    fi
  else
    print_warning "Skipping clap-host build (Qt6 not available)"
  fi
  
  cd "$PROJECT_ROOT"
}

# Build our plugin
build_plugin() {
  print_section "Building Plugin"
  
  # Configure CMake if needed
  if [ ! -d "$BUILD_DIR" ]; then
    print_status "Configuring CMake build..."
    cmake -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug -DCSD_INCLUDE_GUI=FALSE
  fi
  
  # Build the plugin with debug symbols
  print_status "Building plugin with debug symbols..."
  if ! cmake --build "$BUILD_DIR" --config Debug; then
    print_error "Failed to build plugin"
    exit 1
  fi
  print_success "Plugin built successfully"
}

# Inspect plugin with clap-info
inspect_plugin() {
  print_section "Plugin Inspection (clap-info)"
  
  local plugin_path="$BUILD_DIR/clap-saw-demo.clap"
  local clap_info="$TOOLS_DIR/clap-info/build/clap-info"
  
  if [ ! -f "$clap_info" ]; then
    print_warning "clap-info not built, skipping inspection"
    return
  fi
  
  if [ ! -d "$plugin_path" ]; then
    print_error "Plugin not found at: $plugin_path"
    return
  fi
  
  print_status "Inspecting plugin metadata..."
  echo ""
  
  # Basic plugin information
  echo -e "${YELLOW}--- Plugin Descriptors ---${NC}"
  "$clap_info" "$plugin_path" --descriptors-only || print_warning "Failed to get descriptors"
  
  echo ""
  echo -e "${YELLOW}--- Plugin Extensions ---${NC}"
  "$clap_info" "$plugin_path" --extensions || print_warning "Failed to get extensions"
  
  echo ""
  echo -e "${YELLOW}--- Audio Ports ---${NC}"
  "$clap_info" "$plugin_path" --audio-ports || print_warning "Failed to get audio ports"
  
  echo ""
  echo -e "${YELLOW}--- Note Ports ---${NC}"
  "$clap_info" "$plugin_path" --note-ports || print_warning "Failed to get note ports"
  
  echo ""
  echo -e "${YELLOW}--- Parameters ---${NC}"
  "$clap_info" "$plugin_path" --params || print_warning "Failed to get parameters"
}

# Test with clap-validator
validate_plugin() {
  print_section "CLAP Specification Validation"
  
  cd "$PROJECT_ROOT/libs/clap-validator"
  print_status "Running comprehensive CLAP validation..."
  
  local plugin_path="$BUILD_DIR/clap-saw-demo.clap"
  
  # Run all tests, not just failed ones, to get complete picture
  if cargo run --release -- validate "$plugin_path"; then
    print_success "Plugin passes CLAP specification validation"
  else
    print_error "Plugin failed CLAP specification validation"
    return 1
  fi
  
  cd "$PROJECT_ROOT"
}

# Test with clap-host
test_with_host() {
  print_section "Reference Host Testing (clap-host)"
  
  local plugin_path="$BUILD_DIR/clap-saw-demo.clap"
  
  # Find clap-host binary
  local clap_host=""
  if [ -f "$TOOLS_DIR/clap-host/builds/ninja-system/host/Debug/clap-host" ]; then
    clap_host="$TOOLS_DIR/clap-host/builds/ninja-system/host/Debug/clap-host"
  elif [ -f "$TOOLS_DIR/clap-host/build/host/clap-host" ]; then
    clap_host="$TOOLS_DIR/clap-host/build/host/clap-host"
  else
    print_warning "clap-host not found, skipping reference host testing"
    print_status "To enable clap-host testing, install Qt6 and rebuild tools"
    return
  fi
  
  print_status "Testing plugin in reference host..."
  print_status "Host binary: $clap_host"
  print_status "Plugin path: $plugin_path"
  
  # Test if the plugin loads without crashing
  print_status "Attempting to load plugin in reference host..."
  print_warning "If this hangs or crashes, press Ctrl+C and check the debug output above"
  print_status "Use keyboard keys Q-I for testing notes (C3-B3) if plugin loads successfully"
  
  echo ""
  echo -e "${YELLOW}--- Reference Host Output ---${NC}"
  
  # Run with timeout to prevent hanging
  timeout 30s "$clap_host" -p "$plugin_path" || {
    local exit_code=$?
    if [ $exit_code -eq 124 ]; then
      print_warning "clap-host test timed out after 30 seconds (this might be normal for GUI apps)"
    else
      print_error "clap-host failed with exit code: $exit_code"
    fi
  }
}

# Generate debug report
generate_report() {
  print_section "Debug Report Generation"
  
  local report_file="$PROJECT_ROOT/debug-report.txt"
  
  print_status "Generating comprehensive debug report..."
  
  {
    echo "CLAP Saw Demo Plugin Debug Report"
    echo "Generated: $(date)"
    echo "=================================="
    echo ""
    
    echo "System Information:"
    echo "- OS: $(uname -s)"
    echo "- Architecture: $(uname -m)" 
    echo "- CMake: $(cmake --version | head -n1)"
    echo "- Rust: $(cargo --version)"
    
    echo ""
    echo "Build Configuration:"
    echo "- Build Directory: $BUILD_DIR"
    echo "- Plugin Path: $BUILD_DIR/clap-saw-demo.clap"
    echo "- Build Type: Debug"
    
    echo ""
    echo "Plugin File Structure:"
    if [ -d "$BUILD_DIR/clap-saw-demo.clap" ]; then
      find "$BUILD_DIR/clap-saw-demo.clap" -type f | head -10
    else
      echo "Plugin not found!"
    fi
    
  } > "$report_file"
  
  print_success "Debug report saved to: $report_file"
}

# Main execution flow
main() {
  print_section "Starting Comprehensive Plugin Debug Session"
  
  check_dependencies
  build_tools
  build_plugin
  inspect_plugin
  validate_plugin
  test_with_host
  generate_report
  
  print_section "Debug Summary"
  print_success "Debugging session complete!"
  print_status "If plugin crashes in DAW but passes these tests, the issue is likely:"
  print_status "1. Thread safety in DAW integration"
  print_status "2. Missing CLAP extensions that DAW requires"
  print_status "3. GUI-related issues (try building with GUI disabled)"
  print_status "4. Host-specific behavior differences"
  
  echo ""
  print_status "Next steps for Bitwig crash debugging:"
  print_status "1. Check if plugin loads in clap-host (reference implementation)"
  print_status "2. Compare clap-info output with working plugins"
  print_status "3. Test with other CLAP hosts (if available)"
  print_status "4. Enable crash logging in Bitwig preferences"
}

# Run main function
main "$@" 