#!/bin/bash

# E2E Test Runner Script
# Runs end-to-end integration tests against live Home Assistant instance

set -e  # Exit on error
set -u  # Exit on undefined variable

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
HA_URL=""
HA_TOKEN=""
DEVICE_NAME="bed_presence_detector"
REPORT_FILE="e2e-report.xml"
TEST_DIR=""
VERBOSE=false
PYTEST_ARGS=""

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --ha-url)
      HA_URL="$2"
      shift 2
      ;;
    --ha-token)
      HA_TOKEN="$2"
      shift 2
      ;;
    --device-name)
      DEVICE_NAME="$2"
      shift 2
      ;;
    --report-file)
      REPORT_FILE="$2"
      shift 2
      ;;
    --test-dir)
      TEST_DIR="$2"
      shift 2
      ;;
    --verbose)
      VERBOSE=true
      shift
      ;;
    --pytest-args)
      PYTEST_ARGS="$2"
      shift 2
      ;;
    --help)
      echo "Usage: $0 --ha-url <url> --ha-token <token> [options]"
      echo ""
      echo "Options:"
      echo "  --ha-url <url>          Home Assistant WebSocket URL (required)"
      echo "                          Example: ws://homeassistant.local:8123/api/websocket"
      echo "  --ha-token <token>      Long-lived access token (required)"
      echo "  --device-name <name>    ESPHome device name (default: bed_presence_detector)"
      echo "  --report-file <file>    JUnit XML report file (default: e2e-report.xml)"
      echo "  --test-dir <dir>        Test directory (default: tests/e2e)"
      echo "  --verbose               Enable verbose output"
      echo "  --pytest-args <args>    Additional pytest arguments"
      echo "  --help                  Show this help message"
      exit 0
      ;;
    *)
      echo -e "${RED}Unknown option: $1${NC}"
      exit 1
      ;;
  esac
done

# Validate required parameters
if [ -z "$HA_URL" ] || [ -z "$HA_TOKEN" ]; then
  echo -e "${RED}Error: --ha-url and --ha-token are required${NC}"
  exit 1
fi

# Auto-detect test directory if not specified
if [ -z "$TEST_DIR" ]; then
  # Try to find tests/e2e relative to script location
  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  REPO_ROOT="$(dirname "$SCRIPT_DIR")"
  TEST_DIR="$REPO_ROOT/tests/e2e"
fi

if [ ! -d "$TEST_DIR" ]; then
  echo -e "${RED}Error: Test directory not found: $TEST_DIR${NC}"
  exit 1
fi

echo -e "${GREEN}=== E2E Test Runner ===${NC}"
echo "HA URL: $HA_URL"
echo "Device: $DEVICE_NAME"
echo "Test Dir: $TEST_DIR"
echo "Report: $REPORT_FILE"

# Function to check if pytest is available
check_pytest() {
  if ! command -v pytest &> /dev/null; then
    echo -e "${RED}pytest is not installed${NC}"
    echo "Install with: pip install pytest pytest-asyncio"
    return 1
  fi

  echo -e "${GREEN}pytest found: $(pytest --version | head -1)${NC}"
  return 0
}

# Function to check test dependencies
check_dependencies() {
  echo -e "${YELLOW}Checking test dependencies...${NC}"

  local requirements_file="$TEST_DIR/requirements.txt"

  if [ ! -f "$requirements_file" ]; then
    echo -e "${YELLOW}Warning: requirements.txt not found in $TEST_DIR${NC}"
    return 0
  fi

  # Check if key dependencies are available
  local missing_deps=()

  if ! python3 -c "import homeassistant_api" 2>/dev/null; then
    missing_deps+=("homeassistant-api")
  fi

  if [ ${#missing_deps[@]} -gt 0 ]; then
    echo -e "${YELLOW}Missing dependencies: ${missing_deps[*]}${NC}"
    echo "Installing dependencies from $requirements_file..."
    pip install -q -r "$requirements_file"
    echo -e "${GREEN}Dependencies installed${NC}"
  else
    echo -e "${GREEN}All dependencies satisfied${NC}"
  fi
}

# Function to verify HA connectivity before tests
verify_ha_connectivity() {
  echo -e "${YELLOW}Verifying Home Assistant connectivity...${NC}"

  # Extract host and port from WebSocket URL
  local ha_http_url
  ha_http_url=$(echo "$HA_URL" | sed 's|ws://|http://|' | sed 's|/api/websocket||')

  # Try to reach HA API
  local response
  response=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Authorization: Bearer $HA_TOKEN" \
    -H "Content-Type: application/json" \
    "${ha_http_url}/api/" 2>/dev/null || echo "000")

  if [ "$response" = "200" ]; then
    echo -e "${GREEN}Home Assistant is reachable${NC}"
    return 0
  else
    echo -e "${RED}Cannot reach Home Assistant (HTTP $response)${NC}"
    echo "URL: ${ha_http_url}/api/"
    return 1
  fi
}

# Function to verify device exists
verify_device() {
  echo -e "${YELLOW}Verifying ESPHome device '$DEVICE_NAME'...${NC}"

  local ha_http_url
  ha_http_url=$(echo "$HA_URL" | sed 's|ws://|http://|' | sed 's|/api/websocket||')

  # Check if key entities exist
  local entities=(
    "binary_sensor.bed_occupied"
    "number.k_on_on_threshold_multiplier"
    "number.k_off_off_threshold_multiplier"
  )

  local all_found=true

  for entity in "${entities[@]}"; do
    local state
    state=$(curl -s \
      -H "Authorization: Bearer $HA_TOKEN" \
      -H "Content-Type: application/json" \
      "${ha_http_url}/api/states/$entity" 2>/dev/null | grep -o '"state":"[^"]*"' | cut -d'"' -f4 || echo "not_found")

    if [ "$state" = "not_found" ]; then
      echo -e "  ${RED}✗ Missing: $entity${NC}"
      all_found=false
    else
      echo -e "  ${GREEN}✓ Found: $entity${NC}"
    fi
  done

  if [ "$all_found" = true ]; then
    echo -e "${GREEN}Device is integrated and ready${NC}"
    return 0
  else
    echo -e "${RED}Device is not fully integrated${NC}"
    return 1
  fi
}

# Function to run pytest
run_tests() {
  echo -e "${YELLOW}Running E2E tests...${NC}"
  echo ""

  # Set environment variables for tests
  export HA_URL="$HA_URL"
  export HA_TOKEN="$HA_TOKEN"
  export DEVICE_NAME="$DEVICE_NAME"

  # Build pytest command
  local pytest_cmd="pytest"

  # Add verbosity
  if [ "$VERBOSE" = true ]; then
    pytest_cmd="$pytest_cmd -v"
  fi

  # Add JUnit XML report
  pytest_cmd="$pytest_cmd --junit-xml=$REPORT_FILE"

  # Add test directory
  pytest_cmd="$pytest_cmd $TEST_DIR"

  # Add any additional arguments
  if [ -n "$PYTEST_ARGS" ]; then
    pytest_cmd="$pytest_cmd $PYTEST_ARGS"
  fi

  # Run tests
  cd "$(dirname "$TEST_DIR")"

  echo "Command: $pytest_cmd"
  echo ""

  if eval "$pytest_cmd"; then
    echo ""
    echo -e "${GREEN}All tests passed ✓${NC}"
    return 0
  else
    local exit_code=$?
    echo ""
    echo -e "${RED}Some tests failed ✗${NC}"
    return $exit_code
  fi
}

# Function to generate test summary
generate_summary() {
  local exit_code=$1

  echo ""
  echo -e "${GREEN}=== Test Summary ===${NC}"

  if [ -f "$REPORT_FILE" ]; then
    # Parse JUnit XML for summary
    local total_tests
    local failures
    local errors
    local skipped

    total_tests=$(grep -o 'tests="[0-9]*"' "$REPORT_FILE" | head -1 | grep -o '[0-9]*' || echo "0")
    failures=$(grep -o 'failures="[0-9]*"' "$REPORT_FILE" | head -1 | grep -o '[0-9]*' || echo "0")
    errors=$(grep -o 'errors="[0-9]*"' "$REPORT_FILE" | head -1 | grep -o '[0-9]*' || echo "0")
    skipped=$(grep -o 'skipped="[0-9]*"' "$REPORT_FILE" | head -1 | grep -o '[0-9]*' || echo "0")

    echo "Total Tests: $total_tests"
    echo "Passed: $((total_tests - failures - errors - skipped))"
    echo "Failed: $failures"
    echo "Errors: $errors"
    echo "Skipped: $skipped"
    echo ""
    echo "Report: $REPORT_FILE"
  else
    echo -e "${YELLOW}No test report generated${NC}"
  fi

  if [ $exit_code -eq 0 ]; then
    echo ""
    echo -e "${GREEN}Status: SUCCESS ✓${NC}"
  else
    echo ""
    echo -e "${RED}Status: FAILED ✗${NC}"
  fi
}

# Main test execution logic
main() {
  local exit_code=0

  # Step 1: Check pytest
  if ! check_pytest; then
    exit 1
  fi

  # Step 2: Check dependencies
  check_dependencies

  # Step 3: Verify HA connectivity
  if ! verify_ha_connectivity; then
    echo -e "${RED}Cannot proceed with tests - Home Assistant not reachable${NC}"
    exit 1
  fi

  # Step 4: Verify device
  if ! verify_device; then
    echo -e "${YELLOW}Warning: Device verification failed${NC}"
    echo -e "${YELLOW}Tests may fail if device is not properly integrated${NC}"
    echo ""
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
      exit 1
    fi
  fi

  # Step 5: Run tests
  echo ""
  if run_tests; then
    exit_code=0
  else
    exit_code=$?
  fi

  # Step 6: Generate summary
  generate_summary $exit_code

  exit $exit_code
}

# Run main function
main
