#!/bin/bash

# OpenRTX Dynamic Security Analysis Script
# Runtime security testing with sanitizers and valgrind

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create reports directory
mkdir -p security_reports/dynamic
cd security_reports/dynamic

echo -e "${BLUE}🔍 Starting OpenRTX Dynamic Security Analysis${NC}"
echo "============================================="

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Build with AddressSanitizer
build_with_asan() {
    echo -e "${YELLOW}Building with AddressSanitizer...${NC}"
    
    if [ -f "../../meson.build" ]; then
        cd ../..
        meson setup build-asan --buildtype=debug -Dasan=true
        meson compile -C build-asan
        cd security_reports/dynamic
        echo -e "${GREEN}✅ AddressSanitizer build completed${NC}"
    else
        echo -e "${RED}❌ Meson build file not found${NC}"
    fi
}

# Build with UndefinedBehaviorSanitizer
build_with_ubsan() {
    echo -e "${YELLOW}Building with UndefinedBehaviorSanitizer...${NC}"
    
    if [ -f "../../meson.build" ]; then
        cd ../..
        meson setup build-ubsan --buildtype=debug -Dubsan=true
        meson compile -C build-ubsan
        cd security_reports/dynamic
        echo -e "${GREEN}✅ UndefinedBehaviorSanitizer build completed${NC}"
    else
        echo -e "${RED}❌ Meson build file not found${NC}"
    fi
}

# Run valgrind analysis
run_valgrind() {
    echo -e "${YELLOW}Running Valgrind memory analysis...${NC}"
    
    if command_exists valgrind; then
        if [ -f "../../build-asan/openrtx_linux" ]; then
            valgrind --leak-check=full \
                     --show-leak-kinds=all \
                     --track-origins=yes \
                     --verbose \
                     --log-file=valgrind-report.txt \
                     ../../build-asan/openrtx_linux 2>&1 || true
            echo -e "${GREEN}✅ Valgrind analysis completed${NC}"
        else
            echo -e "${RED}❌ Executable not found. Please build first.${NC}"
        fi
    else
        echo -e "${RED}❌ Valgrind not found${NC}"
    fi
}

# Run AddressSanitizer tests
run_asan_tests() {
    echo -e "${YELLOW}Running AddressSanitizer tests...${NC}"
    
    if [ -f "../../build-asan/openrtx_linux" ]; then
        # Set environment variables for AddressSanitizer
        export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:check_initialization_order=1"
        export MSAN_OPTIONS="abort_on_error=1"
        
        # Run the executable and capture output
        timeout 30s ../../build-asan/openrtx_linux > asan-output.txt 2>&1 || true
        
        echo -e "${GREEN}✅ AddressSanitizer tests completed${NC}"
        echo "   Output: asan-output.txt"
    else
        echo -e "${RED}❌ AddressSanitizer executable not found${NC}"
    fi
}

# Run UndefinedBehaviorSanitizer tests
run_ubsan_tests() {
    echo -e "${YELLOW}Running UndefinedBehaviorSanitizer tests...${NC}"
    
    if [ -f "../../build-ubsan/openrtx_linux" ]; then
        # Set environment variables for UBSanitizer
        export UBSAN_OPTIONS="abort_on_error=1:print_stacktrace=1"
        
        # Run the executable and capture output
        timeout 30s ../../build-ubsan/openrtx_linux > ubsan-output.txt 2>&1 || true
        
        echo -e "${GREEN}✅ UndefinedBehaviorSanitizer tests completed${NC}"
        echo "   Output: ubsan-output.txt"
    else
        echo -e "${RED}❌ UndefinedBehaviorSanitizer executable not found${NC}"
    fi
}

# Run ThreadSanitizer tests
run_tsan_tests() {
    echo -e "${YELLOW}Running ThreadSanitizer tests...${NC}"
    
    # Build with ThreadSanitizer
    if [ -f "../../meson.build" ]; then
        cd ../..
        meson setup build-tsan --buildtype=debug -Dc_args="-fsanitize=thread" -Dcpp_args="-fsanitize=thread" -Dlink_args="-fsanitize=thread"
        meson compile -C build-tsan
        cd security_reports/dynamic
        
        if [ -f "../../build-tsan/openrtx_linux" ]; then
            # Set environment variables for ThreadSanitizer
            export TSAN_OPTIONS="abort_on_error=1:print_stacktrace=1"
            
            # Run the executable and capture output
            timeout 30s ../../build-tsan/openrtx_linux > tsan-output.txt 2>&1 || true
            
            echo -e "${GREEN}✅ ThreadSanitizer tests completed${NC}"
            echo "   Output: tsan-output.txt"
        else
            echo -e "${RED}❌ ThreadSanitizer executable not found${NC}"
        fi
    else
        echo -e "${RED}❌ Meson build file not found${NC}"
    fi
}

# Generate dynamic analysis report
generate_dynamic_report() {
    echo -e "${YELLOW}Generating dynamic analysis report...${NC}"
    
    cat > dynamic_analysis_report.md << 'EOF'
# OpenRTX Dynamic Security Analysis Report

**Generated:** $(date)  
**Analysis Type:** Runtime Security Testing  
**Tools Used:** AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer, Valgrind

## Executive Summary

This report contains the results of dynamic security analysis performed on the OpenRTX firmware using runtime instrumentation and memory analysis tools.

## Analysis Results

### AddressSanitizer (ASan)
- **Purpose:** Detects memory errors, buffer overflows, use-after-free
- **Build Configuration:** Debug build with ASan instrumentation
- **Results:** See asan-output.txt for detailed findings

### UndefinedBehaviorSanitizer (UBSan)
- **Purpose:** Detects undefined behavior, integer overflow, type confusion
- **Build Configuration:** Debug build with UBSan instrumentation
- **Results:** See ubsan-output.txt for detailed findings

### ThreadSanitizer (TSan)
- **Purpose:** Detects race conditions and data races
- **Build Configuration:** Debug build with TSan instrumentation
- **Results:** See tsan-output.txt for detailed findings

### Valgrind
- **Purpose:** Memory leak detection, use-after-free, double-free
- **Configuration:** Full leak check with origin tracking
- **Results:** See valgrind-report.txt for detailed findings

## Key Findings

### Memory Issues
- Review AddressSanitizer output for buffer overflows
- Check Valgrind output for memory leaks
- Analyze use-after-free patterns

### Undefined Behavior
- Review UBSanitizer output for undefined behavior
- Check for integer overflows
- Analyze type confusion issues

### Thread Safety
- Review ThreadSanitizer output for race conditions
- Check for data races
- Analyze synchronization issues

## Recommendations

1. **Immediate Action Required**
   - Fix all memory errors found by AddressSanitizer
   - Address undefined behavior found by UBSanitizer
   - Fix race conditions found by ThreadSanitizer

2. **Memory Management**
   - Implement proper memory allocation/deallocation
   - Use RAII patterns where possible
   - Add memory leak detection in CI/CD

3. **Thread Safety**
   - Implement proper synchronization
   - Use mutex/lock mechanisms
   - Review all shared resource access

## Files Generated

- `asan-output.txt`: AddressSanitizer analysis results
- `ubsan-output.txt`: UndefinedBehaviorSanitizer analysis results
- `tsan-output.txt`: ThreadSanitizer analysis results
- `valgrind-report.txt`: Valgrind memory analysis results

## Next Steps

1. Review all generated reports
2. Fix critical memory and thread safety issues
3. Implement continuous dynamic analysis in CI/CD
4. Regular runtime security testing

---

*This report was generated using automated dynamic analysis tools. Manual review and testing are recommended for critical security decisions.*
EOF

    echo -e "${GREEN}✅ Dynamic analysis report generated${NC}"
    echo "   Report: dynamic_analysis_report.md"
}

# Main execution
main() {
    echo "OpenRTX Dynamic Security Analysis - $(date)"
    echo "============================================="
    
    # Check if we're in the right directory
    if [ ! -f "../../meson.build" ]; then
        echo -e "${RED}Error: Please run this script from the OpenRTX project root directory${NC}"
        exit 1
    fi
    
    build_with_asan
    build_with_ubsan
    run_valgrind
    run_asan_tests
    run_ubsan_tests
    run_tsan_tests
    generate_dynamic_report
    
    echo -e "${GREEN}🎉 Dynamic security analysis completed successfully!${NC}"
    echo "All reports are available in the security_reports/dynamic/ directory"
}

# Run main function
main "$@"
