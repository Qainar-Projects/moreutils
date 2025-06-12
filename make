#!/bin/bash
#
# build.sh - QCO MoreUtils Build Script
# Copyright 2025 AnmiTaliDev
# Licensed under the Apache License, Version 2.0
#
# Simple build script for all utilities in the QCO MoreUtils package
#

set -e  # Exit on any error

# Configuration
PROJECT_NAME="QCO MoreUtils"
VERSION="1.0.0"
AUTHOR="AnmiTaliDev"

# Directories
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$ROOT_DIR/src"
BIN_DIR="$ROOT_DIR/bin"

# Compiler settings
CC="${CC:-gcc}"
CXX="${CXX:-g++}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Utility definitions
declare -A UTILITIES=(
    # C utilities
    ["kill"]="c"
    ["stat"]="c"
    ["tee"]="c"
    ["yes"]="c"
    
    # C++ utilities
    ["conf-convert"]="cpp"
    ["date"]="cpp"
    ["lower"]="cpp"
    ["no"]="cpp"
    ["ping"]="cpp"
    ["sleep"]="cpp"
    ["tree"]="cpp"
    ["upper"]="cpp"
    ["uptime"]="cpp"
    ["whois"]="cpp"
)

# Functions
print_header() {
    echo -e "${CYAN}======================================${NC}"
    echo -e "${CYAN}  $PROJECT_NAME Build Script${NC}"
    echo -e "${CYAN}  Version: $VERSION${NC}"
    echo -e "${CYAN}  Author: $AUTHOR${NC}"
    echo -e "${CYAN}======================================${NC}"
    echo
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_dependencies() {
    print_info "Checking build dependencies..."
    
    # Check for required compilers
    if ! command -v "$CC" &> /dev/null; then
        print_error "C compiler '$CC' not found"
        exit 1
    fi
    
    if ! command -v "$CXX" &> /dev/null; then
        print_error "C++ compiler '$CXX' not found"
        exit 1
    fi
    
    print_success "All dependencies found"
    echo "  CC: $($CC --version | head -n1)"
    echo "  CXX: $($CXX --version | head -n1)"
    echo
}

setup_directories() {
    print_info "Setting up build directories..."
    
    # Create directories if they don't exist
    mkdir -p "$BIN_DIR"
    
    print_success "Directories ready"
    echo
}

clean_build() {
    print_info "Cleaning previous build artifacts..."
    
    # Clean bin directory
    if [ -d "$BIN_DIR" ]; then
        rm -f "$BIN_DIR"/*
    fi
    
    print_success "Clean completed"
    echo
}

build_utility() {
    local name="$1"
    local lang="$2"
    
    print_info "Building $name..."
    
    local src_file=""
    local compiler=""
    local output="$BIN_DIR/$name"
    
    # Determine source file and compiler
    if [ "$lang" = "c" ]; then
        src_file="$SRC_DIR/$name/main.c"
        compiler="$CC"
    elif [ "$lang" = "cpp" ]; then
        src_file="$SRC_DIR/$name/main.cpp"
        compiler="$CXX"
    else
        print_error "Unknown language: $lang"
        return 1
    fi
    
    # Check if source file exists
    if [ ! -f "$src_file" ]; then
        print_error "Source file not found: $src_file"
        return 1
    fi
    
    # Build the utility (simple command)
    if $compiler -O3 "$src_file" -o "$output"; then
        print_success "Built $name"
        
        # Make executable
        chmod +x "$output"
        
        # Show file size
        local size=$(stat -c%s "$output" 2>/dev/null || stat -f%z "$output" 2>/dev/null || echo "unknown")
        echo "  Size: $size bytes"
        
        return 0
    else
        print_error "Failed to build $name"
        return 1
    fi
}

build_all() {
    print_info "Building all utilities..."
    echo
    
    local total=${#UTILITIES[@]}
    local built=0
    local failed=0
    
    for utility in "${!UTILITIES[@]}"; do
        local lang="${UTILITIES[$utility]}"
        
        if build_utility "$utility" "$lang"; then
            ((built++))
        else
            ((failed++))
        fi
        echo
    done
    
    echo -e "${CYAN}======================================${NC}"
    echo -e "${CYAN}  Build Summary${NC}"
    echo -e "${CYAN}======================================${NC}"
    echo "Total utilities: $total"
    echo -e "Built successfully: ${GREEN}$built${NC}"
    echo -e "Failed: ${RED}$failed${NC}"
    echo
    
    if [ $failed -eq 0 ]; then
        print_success "All utilities built successfully!"
        return 0
    else
        print_error "$failed utilities failed to build"
        return 1
    fi
}

show_help() {
    echo "Usage: $0 [OPTIONS] [TARGETS]"
    echo
    echo "OPTIONS:"
    echo "  -h, --help          Show this help message"
    echo "  -c, --clean         Clean build artifacts before building"
    echo
    echo "TARGETS:"
    echo "  all                 Build all utilities (default)"
    echo "  clean               Clean build artifacts"
    echo "  UTILITY_NAME        Build specific utility"
    echo
    echo "UTILITIES:"
    for utility in $(printf '%s\n' "${!UTILITIES[@]}" | sort); do
        echo "  $utility"
    done
    echo
    echo "EXAMPLES:"
    echo "  $0                  # Build all utilities"
    echo "  $0 --clean all      # Clean and build all"
    echo "  $0 date ping        # Build only date and ping"
    echo
}

# Parse command line arguments
TARGETS=()
CLEAN_FIRST=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN_FIRST=true
            shift
            ;;
        clean)
            TARGETS+=("clean")
            shift
            ;;
        all)
            TARGETS+=("all")
            shift
            ;;
        *)
            # Check if it's a valid utility name
            if [[ "${UTILITIES[$1]}" ]]; then
                TARGETS+=("$1")
            else
                print_error "Unknown option or utility: $1"
                echo "Use --help for usage information"
                exit 1
            fi
            shift
            ;;
    esac
done

# Default target is "all"
if [ ${#TARGETS[@]} -eq 0 ]; then
    TARGETS=("all")
fi

# Main execution
main() {
    print_header
    
    # Handle special targets first
    for target in "${TARGETS[@]}"; do
        case $target in
            clean)
                clean_build
                exit 0
                ;;
        esac
    done
    
    # Check dependencies
    check_dependencies
    
    # Setup directories
    setup_directories
    
    # Clean if requested
    if [ "$CLEAN_FIRST" = true ]; then
        clean_build
    fi
    
    # Build targets
    local success=true
    for target in "${TARGETS[@]}"; do
        case $target in
            all)
                if ! build_all; then
                    success=false
                fi
                ;;
            *)
                if [[ "${UTILITIES[$target]}" ]]; then
                    if ! build_utility "$target" "${UTILITIES[$target]}"; then
                        success=false
                    fi
                    echo
                else
                    print_error "Unknown utility: $target"
                    success=false
                fi
                ;;
        esac
    done
    
    # Final status
    if [ "$success" = true ]; then
        print_success "Build completed successfully!"
        echo
        print_info "Built utilities are available in: $BIN_DIR"
        exit 0
    else
        print_error "Build completed with errors"
        exit 1
    fi
}

# Run main function
main "$@"