#!/bin/bash

# ESPHome Deployment Script
# Deploys firmware to M5Stack device via OTA or USB

set -e  # Exit on error
set -u  # Exit on undefined variable

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
DEVICE_HOST=""
OTA_PASSWORD=""
CONFIG_FILE=""
DEPLOYMENT_METHOD="ota"  # ota, usb, or upload-only
USB_PORT=""
VERIFY_TIMEOUT=60

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --device)
      DEVICE_HOST="$2"
      shift 2
      ;;
    --password)
      OTA_PASSWORD="$2"
      shift 2
      ;;
    --config)
      CONFIG_FILE="$2"
      shift 2
      ;;
    --method)
      DEPLOYMENT_METHOD="$2"
      shift 2
      ;;
    --usb-port)
      USB_PORT="$2"
      shift 2
      ;;
    --timeout)
      VERIFY_TIMEOUT="$2"
      shift 2
      ;;
    --help)
      echo "Usage: $0 --device <host> --password <pwd> --config <yaml> [options]"
      echo ""
      echo "Options:"
      echo "  --device <host>       Device hostname or IP (required for OTA)"
      echo "  --password <pwd>      OTA password (required for OTA)"
      echo "  --config <yaml>       ESPHome config file path (required)"
      echo "  --method <method>     Deployment method: ota, usb, upload-only (default: ota)"
      echo "  --usb-port <port>     USB serial port (required for USB method)"
      echo "  --timeout <seconds>   Verification timeout (default: 60)"
      echo "  --help                Show this help message"
      exit 0
      ;;
    *)
      echo -e "${RED}Unknown option: $1${NC}"
      exit 1
      ;;
  esac
done

# Validate required parameters
if [ -z "$CONFIG_FILE" ]; then
  echo -e "${RED}Error: --config is required${NC}"
  exit 1
fi

if [ ! -f "$CONFIG_FILE" ]; then
  echo -e "${RED}Error: Config file not found: $CONFIG_FILE${NC}"
  exit 1
fi

if [ "$DEPLOYMENT_METHOD" = "ota" ]; then
  if [ -z "$DEVICE_HOST" ] || [ -z "$OTA_PASSWORD" ]; then
    echo -e "${RED}Error: --device and --password are required for OTA deployment${NC}"
    exit 1
  fi
fi

if [ "$DEPLOYMENT_METHOD" = "usb" ]; then
  if [ -z "$USB_PORT" ]; then
    echo -e "${RED}Error: --usb-port is required for USB deployment${NC}"
    exit 1
  fi
fi

echo -e "${GREEN}=== ESPHome Deployment ===${NC}"
echo "Config: $CONFIG_FILE"
echo "Method: $DEPLOYMENT_METHOD"

# Function to check if device is reachable
check_device_reachable() {
  local host=$1
  echo -e "${YELLOW}Checking if device is reachable...${NC}"
  if ping -c 3 -W 2 "$host" > /dev/null 2>&1; then
    echo -e "${GREEN}Device is reachable at $host${NC}"
    return 0
  else
    echo -e "${RED}Device is not reachable at $host${NC}"
    return 1
  fi
}

# Function to compile firmware
compile_firmware() {
  echo -e "${YELLOW}Compiling firmware...${NC}"

  # Get the directory of the config file
  CONFIG_DIR=$(dirname "$CONFIG_FILE")
  CONFIG_NAME=$(basename "$CONFIG_FILE")

  cd "$CONFIG_DIR"

  if esphome compile "$CONFIG_NAME"; then
    echo -e "${GREEN}Firmware compiled successfully${NC}"
    return 0
  else
    echo -e "${RED}Firmware compilation failed${NC}"
    return 1
  fi
}

# Function to upload via OTA
upload_ota() {
  local host=$1
  local password=$2

  echo -e "${YELLOW}Uploading firmware via OTA to $host...${NC}"

  # Check if device is reachable first
  if ! check_device_reachable "$host"; then
    echo -e "${RED}Cannot proceed with OTA upload - device not reachable${NC}"
    return 1
  fi

  CONFIG_DIR=$(dirname "$CONFIG_FILE")
  CONFIG_NAME=$(basename "$CONFIG_FILE")

  cd "$CONFIG_DIR"

  # Use esphome upload command with OTA
  if esphome upload "$CONFIG_NAME" --device "$host"; then
    echo -e "${GREEN}OTA upload successful${NC}"
    return 0
  else
    echo -e "${RED}OTA upload failed${NC}"
    return 1
  fi
}

# Function to upload via USB
upload_usb() {
  local port=$1

  echo -e "${YELLOW}Uploading firmware via USB to $port...${NC}"

  if [ ! -e "$port" ]; then
    echo -e "${RED}USB port not found: $port${NC}"
    return 1
  fi

  CONFIG_DIR=$(dirname "$CONFIG_FILE")
  CONFIG_NAME=$(basename "$CONFIG_FILE")

  cd "$CONFIG_DIR"

  if esphome upload "$CONFIG_NAME" --device "$port"; then
    echo -e "${GREEN}USB upload successful${NC}"
    return 0
  else
    echo -e "${RED}USB upload failed${NC}"
    return 1
  fi
}

# Function to verify deployment
verify_deployment() {
  local host=$1

  echo -e "${YELLOW}Verifying deployment...${NC}"

  # Wait for device to come back online
  echo "Waiting for device to boot (this may take up to ${VERIFY_TIMEOUT}s)..."

  elapsed=0
  while ! ping -c 1 -W 1 "$host" > /dev/null 2>&1; do
    if [ $elapsed -ge $VERIFY_TIMEOUT ]; then
      echo -e "${RED}Device did not come online within ${VERIFY_TIMEOUT}s${NC}"
      return 1
    fi
    echo -n "."
    sleep 2
    elapsed=$((elapsed + 2))
  done

  echo ""
  echo -e "${GREEN}Device is online and responding${NC}"

  # Try to connect to ESPHome native API to verify it's working
  echo "Verifying ESPHome API connectivity..."
  sleep 5  # Give API time to start

  if timeout 10 bash -c "echo > /dev/tcp/$host/6053" 2>/dev/null; then
    echo -e "${GREEN}ESPHome API is accessible${NC}"
    return 0
  else
    echo -e "${YELLOW}Warning: Could not verify ESPHome API (port 6053)${NC}"
    echo -e "${YELLOW}This may be normal if API encryption is enabled${NC}"
    return 0  # Don't fail on this
  fi
}

# Main deployment logic
main() {
  echo "Starting deployment process..."

  # Step 1: Compile firmware
  if ! compile_firmware; then
    echo -e "${RED}Deployment failed at compilation stage${NC}"
    exit 1
  fi

  # Step 2: Upload firmware
  case $DEPLOYMENT_METHOD in
    ota)
      if ! upload_ota "$DEVICE_HOST" "$OTA_PASSWORD"; then
        echo -e "${RED}Deployment failed at OTA upload stage${NC}"
        exit 1
      fi

      # Step 3: Verify deployment
      if ! verify_deployment "$DEVICE_HOST"; then
        echo -e "${YELLOW}Warning: Deployment verification failed${NC}"
        echo -e "${YELLOW}Device may still be functional, but automated verification couldn't confirm${NC}"
        exit 1
      fi
      ;;

    usb)
      if ! upload_usb "$USB_PORT"; then
        echo -e "${RED}Deployment failed at USB upload stage${NC}"
        exit 1
      fi

      # For USB, we can't easily verify without knowing the IP
      echo -e "${YELLOW}USB upload complete. Device verification skipped (requires network config)${NC}"
      ;;

    upload-only)
      echo -e "${GREEN}Compile-only mode: Firmware compiled successfully${NC}"
      echo "Firmware location: $(dirname $CONFIG_FILE)/.esphome/build/*/firmware.bin"
      ;;

    *)
      echo -e "${RED}Unknown deployment method: $DEPLOYMENT_METHOD${NC}"
      exit 1
      ;;
  esac

  echo ""
  echo -e "${GREEN}=== Deployment Complete ===${NC}"
  echo "Device: ${DEVICE_HOST:-N/A}"
  echo "Method: $DEPLOYMENT_METHOD"
  echo "Status: Success ✓"
}

# Run main function
main
