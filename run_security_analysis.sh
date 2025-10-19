#!/bin/bash

# Simplified OpenRTX Security Analysis Script
# Focus on available tools and comprehensive analysis

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

echo -e "${BLUE}🔍 Starting OpenRTX Security Analysis${NC}"
echo "=========================================="

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Run cppcheck analysis
run_cppcheck() {
    echo -e "${YELLOW}Running cppcheck static analysis...${NC}"
    if command_exists cppcheck; then
        cppcheck --enable=all --inconclusive --std=c++14 \
                 --suppress=missingIncludeSystem \
                 --suppress=unusedFunction \
                 --suppress=unmatchedSuppression \
                 --xml --xml-version=2 \
                 --output-file=cppcheck-report.xml \
                 ../ 2> cppcheck-errors.txt
        
        echo -e "${GREEN}✅ Cppcheck analysis completed${NC}"
        echo "   XML report: cppcheck-report.xml"
        echo "   Errors: cppcheck-errors.txt"
    else
        echo -e "${RED}❌ cppcheck not found${NC}"
    fi
}

# Run flawfinder analysis
run_flawfinder() {
    echo -e "${YELLOW}Running flawfinder security analysis...${NC}"
    if command_exists flawfinder; then
        flawfinder --html --context --minlevel=1 ../ > flawfinder-report.html 2>&1
        echo -e "${GREEN}✅ Flawfinder analysis completed${NC}"
        echo "   HTML report: flawfinder-report.html"
    else
        echo -e "${RED}❌ flawfinder not found${NC}"
    fi
}

# Run custom security pattern analysis
run_custom_analysis() {
    echo -e "${YELLOW}Running custom security pattern analysis...${NC}"
    
    # Buffer overflow patterns
    echo "   Searching for buffer overflow patterns..."
    grep -r -n "strcpy\|strcat\|sprintf\|gets\|scanf" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > buffer-overflow-patterns.txt 2>/dev/null || true
    
    # Memory management issues
    echo "   Searching for memory management issues..."
    grep -r -n "free.*;" ../openrtx/ --include="*.c" --include="*.cpp" -A 2 -B 2 > potential-use-after-free.txt 2>/dev/null || true
    grep -r -n "malloc\|calloc\|realloc" ../openrtx/ --include="*.c" --include="*.cpp" > memory-allocations.txt 2>/dev/null || true
    
    # Thread safety patterns
    echo "   Searching for thread safety patterns..."
    grep -r -n "pthread_\|mutex_\|sem_\|thread" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > thread-patterns.txt 2>/dev/null || true
    grep -r -n "volatile" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > volatile-usage.txt 2>/dev/null || true
    
    # Cryptography patterns
    echo "   Searching for cryptography patterns..."
    grep -r -n "MD5\|SHA1\|DES\|AES\|encrypt\|decrypt" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > crypto-patterns.txt 2>/dev/null || true
    
    # Hardcoded secrets
    echo "   Searching for hardcoded secrets..."
    grep -r -n "password\|secret\|key.*=" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > hardcoded-secrets.txt 2>/dev/null || true
    
    echo -e "${GREEN}✅ Custom pattern analysis completed${NC}"
}

# Run code quality analysis
run_code_quality() {
    echo -e "${YELLOW}Running code quality analysis...${NC}"
    
    # Function complexity analysis
    echo "   Analyzing function complexity..."
    find ../openrtx/ -name "*.c" -o -name "*.cpp" | xargs wc -l > function-complexity.txt 2>/dev/null || true
    
    # Comment density analysis
    echo "   Analyzing comment density..."
    find ../openrtx/ -name "*.c" -o -name "*.cpp" | xargs grep -c "//\|/\*" > comment-density.txt 2>/dev/null || true
    
    # TODO/FIXME analysis
    echo "   Searching for TODO/FIXME comments..."
    grep -r -n "TODO\|FIXME\|HACK\|XXX" ../openrtx/ --include="*.c" --include="*.cpp" --include="*.h" > todo-fixme.txt 2>/dev/null || true
    
    echo -e "${GREEN}✅ Code quality analysis completed${NC}"
}

# Generate comprehensive report
generate_report() {
    echo -e "${YELLOW}Generating comprehensive security report...${NC}"
    
    cat > security_summary.md << 'EOF'
# OpenRTX Security and Code Quality Analysis Report

Generated: $(date)

## Analysis Summary

This report contains the results of comprehensive security and code quality analysis performed on the OpenRTX firmware codebase.

## Tools Used

1. **Cppcheck**: Static analysis for C/C++ code
2. **Flawfinder**: Security-focused static analysis
3. **Custom Pattern Analysis**: Manual pattern detection for security issues
4. **Code Quality Analysis**: Code metrics and quality assessment

## Key Findings

### High Priority Security Issues
- Review cppcheck-report.xml for critical issues
- Check flawfinder-report.html for security vulnerabilities
- Examine buffer-overflow-patterns.txt for unsafe string operations

### Memory Safety Issues
- Review potential-use-after-free.txt for memory management issues
- Check memory-allocations.txt for potential memory leaks

### Thread Safety Issues
- Review thread-patterns.txt for concurrency issues
- Check volatile-usage.txt for proper volatile usage

### Cryptography Issues
- Review crypto-patterns.txt for cryptographic implementations
- Check hardcoded-secrets.txt for exposed secrets

## Recommendations

1. **Immediate Action Required**
   - Fix all critical issues found by cppcheck
   - Address security vulnerabilities identified by flawfinder
   - Replace unsafe string functions with safe alternatives

2. **Short Term Improvements**
   - Implement proper memory management patterns
   - Add thread safety mechanisms where needed
   - Strengthen cryptographic implementations

3. **Long Term Improvements**
   - Implement automated security scanning in CI/CD
   - Add security-focused code reviews
   - Regular security audits

## Files Generated

- `cppcheck-report.xml`: Detailed static analysis results
- `flawfinder-report.html`: Security vulnerability report
- `buffer-overflow-patterns.txt`: Unsafe string operation locations
- `potential-use-after-free.txt`: Memory management issues
- `memory-allocations.txt`: Memory allocation tracking
- `thread-patterns.txt`: Thread-related code patterns
- `crypto-patterns.txt`: Cryptographic code patterns
- `hardcoded-secrets.txt`: Potential hardcoded secrets
- `todo-fixme.txt`: Code improvement markers

## Next Steps

1. Review all generated reports
2. Prioritize fixes based on severity
3. Implement fixes for high-priority issues
4. Re-run analysis to verify improvements
5. Consider implementing continuous security monitoring

EOF

    echo -e "${GREEN}✅ Comprehensive security report generated${NC}"
    echo "   Report: security_summary.md"
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
    
    run_cppcheck
    run_flawfinder
    run_custom_analysis
    run_code_quality
    generate_report
    
    echo -e "${GREEN}🎉 Security analysis completed successfully!${NC}"
    echo "All reports are available in the security_reports/ directory"
}

# Run main function
main "$@"
