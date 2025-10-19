#!/bin/bash

# OpenRTX Security and Code Quality Analysis Script
# Comprehensive analysis using multiple static and dynamic analysis tools

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create reports directory
mkdir -p security_reports
cd security_reports

echo -e "${BLUE}🔍 Starting OpenRTX Security and Code Quality Analysis${NC}"
echo "=================================================="

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install required tools if not present
install_tools() {
    echo -e "${YELLOW}📦 Checking and installing analysis tools...${NC}"
    
    # Update package list
    sudo apt-get update -qq
    
    # Install static analysis tools
    if ! command_exists cppcheck; then
        echo "Installing cppcheck..."
        sudo apt-get install -y cppcheck
    fi
    
    if ! command_exists flawfinder; then
        echo "Installing flawfinder..."
        sudo apt-get install -y flawfinder
    fi
    
    if ! command_exists rats; then
        echo "Installing rats..."
        sudo apt-get install -y rough-auditing-tool-for-security || echo "RATS not available in package manager, skipping..."
    fi
    
    if ! command_exists semgrep; then
        echo "Installing semgrep..."
        pip3 install semgrep
    fi
    
    if ! command_exists valgrind; then
        echo "Installing valgrind..."
        sudo apt-get install -y valgrind
    fi
    
    if ! command_exists scan-build; then
        echo "Installing clang static analyzer..."
        sudo apt-get install -y clang-tools
    fi
}

# Run static analysis tools
run_static_analysis() {
    echo -e "${BLUE}🔍 Running Static Analysis Tools${NC}"
    echo "================================="
    
    # Cppcheck - Comprehensive C/C++ static analysis
    echo -e "${YELLOW}Running cppcheck...${NC}"
    if command_exists cppcheck; then
        cppcheck --enable=all --inconclusive --std=c++14 \
                 --suppress=missingIncludeSystem \
                 --suppress=unusedFunction \
                 --suppress=unmatchedSuppression \
                 --xml --xml-version=2 \
                 --output-file=cppcheck-report.xml \
                 ../ 2> cppcheck-errors.txt
        
        # Generate HTML report
        if [ -f cppcheck-report.xml ]; then
            echo "Cppcheck analysis completed. XML report: cppcheck-report.xml"
        fi
    else
        echo -e "${RED}cppcheck not found. Please install it.${NC}"
    fi
    
    # Flawfinder - Security-focused static analysis
    echo -e "${YELLOW}Running flawfinder...${NC}"
    if command_exists flawfinder; then
        flawfinder --html --context --minlevel=1 ../ > flawfinder-report.html 2>&1
        echo "Flawfinder analysis completed. HTML report: flawfinder-report.html"
    else
        echo -e "${RED}flawfinder not found. Please install it.${NC}"
    fi
    
    # RATS - Rough Auditing Tool for Security
    echo -e "${YELLOW}Running RATS...${NC}"
    if command_exists rats; then
        rats --html ../ > rats-report.html 2>&1
        echo "RATS analysis completed. HTML report: rats-report.html"
    else
        echo -e "${RED}RATS not found. Please install it.${NC}"
    fi
    
    # Clang Static Analyzer
    echo -e "${YELLOW}Running clang static analyzer...${NC}"
    if command_exists scan-build; then
        cd ..
        scan-build -o security_reports/scan-build-report make 2>&1 | tee security_reports/scan-build-output.txt
        cd security_reports
        echo "Clang static analyzer completed. Report in: scan-build-report/"
    else
        echo -e "${RED}clang static analyzer not found. Please install clang-tools.${NC}"
    fi
}

# Run cryptography-specific analysis
run_crypto_analysis() {
    echo -e "${BLUE}🔐 Running Cryptography Security Analysis${NC}"
    echo "============================================="
    
    # Create custom crypto rules
    cat > crypto-rules.yml << 'EOF'
rules:
  - id: key-logging
    pattern: printf(..., $_key, ...)
    message: "CRITICAL: Never log cryptographic keys"
    severity: ERROR
    
  - id: insecure-random
    patterns:
      - pattern: rand()
      - pattern: srand(...)
    message: "Use cryptographically secure RNG"
    severity: WARNING
    
  - id: missing-explicit-bzero
    pattern: memset($PTR, 0, ...)
    message: "Use explicit_bzero for sensitive data"
    severity: WARNING
    
  - id: hardcoded-secrets
    patterns:
      - pattern: "password\s*=\s*[\"'][^\"']*[\"']"
      - pattern: "secret\s*=\s*[\"'][^\"']*[\"']"
      - pattern: "key\s*=\s*[\"'][^\"']*[\"']"
    message: "Hardcoded secrets detected"
    severity: ERROR
    
  - id: weak-crypto
    patterns:
      - pattern: MD5(...)
      - pattern: SHA1(...)
      - pattern: DES(...)
    message: "Weak cryptographic algorithm detected"
    severity: WARNING
    
  - id: crypto-key-size
    patterns:
      - pattern: "key.*=.*{.*}"
    message: "Check key size for cryptographic strength"
    severity: INFO
EOF

    # Run semgrep with crypto rules
    echo -e "${YELLOW}Running semgrep crypto analysis...${NC}"
    if command_exists semgrep; then
        semgrep --config "p/crypto-audit" ../ --json --output=crypto-audit.json
        semgrep --config crypto-rules.yml ../ --json --output=custom-crypto-rules.json
        echo "Cryptography analysis completed. Reports: crypto-audit.json, custom-crypto-rules.json"
    else
        echo -e "${RED}semgrep not found. Please install it.${NC}"
    fi
}

# Run memory safety analysis
run_memory_analysis() {
    echo -e "${BLUE}🧠 Running Memory Safety Analysis${NC}"
    echo "===================================="
    
    # Look for common memory safety issues
    echo -e "${YELLOW}Searching for buffer overflow patterns...${NC}"
    grep -r -n "strcpy\|strcat\|sprintf\|gets\|scanf" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > buffer-overflow-patterns.txt 2>/dev/null || true
    
    echo -e "${YELLOW}Searching for use-after-free patterns...${NC}"
    grep -r -n "free.*;" ../openrtx/ --include="*.c" --include="*.cpp" -A 2 -B 2 > potential-use-after-free.txt 2>/dev/null || true
    
    echo -e "${YELLOW}Searching for double-free patterns...${NC}"
    grep -r -n "free.*free" ../openrtx/ --include="*.c" --include="*.cpp" > potential-double-free.txt 2>/dev/null || true
    
    echo -e "${YELLOW}Searching for memory leaks...${NC}"
    grep -r -n "malloc\|calloc\|realloc" ../openrtx/ --include="*.c" --include="*.cpp" > memory-allocations.txt 2>/dev/null || true
    
    echo "Memory safety analysis completed. Check generated files for patterns."
}

# Run thread safety analysis
run_thread_analysis() {
    echo -e "${BLUE}🧵 Running Thread Safety Analysis${NC}"
    echo "===================================="
    
    # Look for thread safety issues
    echo -e "${YELLOW}Searching for race condition patterns...${NC}"
    grep -r -n "pthread_\|mutex_\|sem_\|thread" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > thread-patterns.txt 2>/dev/null || true
    
    echo -e "${YELLOW}Searching for global variable access...${NC}"
    grep -r -n "^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*=" ../openrtx/src/ --include="*.c" --include="*.cpp" > global-variables.txt 2>/dev/null || true
    
    echo -e "${YELLOW}Searching for volatile keyword usage...${NC}"
    grep -r -n "volatile" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > volatile-usage.txt 2>/dev/null || true
    
    echo "Thread safety analysis completed. Check generated files for patterns."
}

# Generate comprehensive report
generate_report() {
    echo -e "${BLUE}📊 Generating Comprehensive Security Report${NC}"
    echo "============================================="
    
    cat > security_summary.md << 'EOF'
# OpenRTX Security and Code Quality Analysis Report

## Executive Summary

This report contains the results of comprehensive security and code quality analysis performed on the OpenRTX firmware codebase.

## Analysis Tools Used

1. **Static Analysis Tools**
   - Cppcheck: C/C++ static analysis
   - Flawfinder: Security-focused static analysis
   - RATS: Rough Auditing Tool for Security
   - Clang Static Analyzer: Advanced static analysis
   - Semgrep: Custom rule-based analysis

2. **Memory Safety Analysis**
   - Buffer overflow pattern detection
   - Use-after-free pattern detection
   - Double-free pattern detection
   - Memory allocation tracking

3. **Thread Safety Analysis**
   - Race condition pattern detection
   - Global variable access analysis
   - Volatile keyword usage analysis

4. **Cryptography Analysis**
   - Hardcoded secrets detection
   - Weak cryptographic algorithm detection
   - Key management analysis

## Files Generated

- `cppcheck-report.xml`: Cppcheck analysis results
- `flawfinder-report.html`: Flawfinder security analysis
- `rats-report.html`: RATS security analysis
- `scan-build-report/`: Clang static analyzer results
- `crypto-audit.json`: Cryptography security analysis
- `custom-crypto-rules.json`: Custom crypto rule analysis
- `buffer-overflow-patterns.txt`: Potential buffer overflow locations
- `potential-use-after-free.txt`: Potential use-after-free issues
- `potential-double-free.txt`: Potential double-free issues
- `memory-allocations.txt`: Memory allocation tracking
- `thread-patterns.txt`: Thread-related code patterns
- `global-variables.txt`: Global variable usage
- `volatile-usage.txt`: Volatile keyword usage

## Recommendations

1. **High Priority Issues**
   - Review all findings in cppcheck-report.xml
   - Address security issues found by flawfinder and RATS
   - Fix any buffer overflow patterns identified

2. **Medium Priority Issues**
   - Review memory allocation patterns for potential leaks
   - Analyze thread safety of global variable access
   - Strengthen cryptography implementations

3. **Low Priority Issues**
   - Code quality improvements suggested by static analyzers
   - Documentation improvements for complex functions

## Next Steps

1. Review all generated reports
2. Prioritize fixes based on severity
3. Implement fixes for high-priority security issues
4. Re-run analysis after fixes to verify improvements
5. Consider implementing automated security scanning in CI/CD

EOF

    echo -e "${GREEN}✅ Comprehensive security analysis completed!${NC}"
    echo "Reports generated in: $(pwd)"
    echo "Summary report: security_summary.md"
}

# Main execution
main() {
    echo "OpenRTX Security Analysis - $(date)"
    echo "======================================"
    
    # Check if we're in the right directory
    if [ ! -f "../meson.build" ]; then
        echo -e "${RED}Error: Please run this script from the OpenRTX project root directory${NC}"
        exit 1
    fi
    
    install_tools
    run_static_analysis
    run_crypto_analysis
    run_memory_analysis
    run_thread_analysis
    generate_report
    
    echo -e "${GREEN}🎉 All security analysis completed successfully!${NC}"
}

# Run main function
main "$@"
