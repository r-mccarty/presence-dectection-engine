#!/bin/bash

# Home Assistant Configuration Deployment Script
# Deploys dashboards, blueprints, and configuration helpers to Home Assistant

set -e  # Exit on error
set -u  # Exit on undefined variable

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
HA_HOST=""
HA_TOKEN=""
CONFIG_PATH=""
HA_CONFIG_DIR="/config"  # Default HA config directory
DEPLOYMENT_METHOD="api"  # api or filesystem
DRY_RUN=false

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --ha-host)
      HA_HOST="$2"
      shift 2
      ;;
    --ha-token)
      HA_TOKEN="$2"
      shift 2
      ;;
    --config-path)
      CONFIG_PATH="$2"
      shift 2
      ;;
    --ha-config-dir)
      HA_CONFIG_DIR="$2"
      shift 2
      ;;
    --method)
      DEPLOYMENT_METHOD="$2"
      shift 2
      ;;
    --dry-run)
      DRY_RUN=true
      shift
      ;;
    --help)
      echo "Usage: $0 --ha-host <host> --ha-token <token> --config-path <path> [options]"
      echo ""
      echo "Options:"
      echo "  --ha-host <host>        Home Assistant hostname or IP (required)"
      echo "  --ha-token <token>      Long-lived access token (required)"
      echo "  --config-path <path>    Path to HA config files in repo (required)"
      echo "  --ha-config-dir <dir>   HA config directory on host (default: /config)"
      echo "  --method <method>       Deployment method: api or filesystem (default: api)"
      echo "  --dry-run               Show what would be done without making changes"
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
if [ -z "$HA_HOST" ] || [ -z "$HA_TOKEN" ] || [ -z "$CONFIG_PATH" ]; then
  echo -e "${RED}Error: --ha-host, --ha-token, and --config-path are required${NC}"
  exit 1
fi

if [ ! -d "$CONFIG_PATH" ]; then
  echo -e "${RED}Error: Config path not found: $CONFIG_PATH${NC}"
  exit 1
fi

echo -e "${GREEN}=== Home Assistant Deployment ===${NC}"
echo "HA Host: $HA_HOST"
echo "Config Path: $CONFIG_PATH"
echo "Method: $DEPLOYMENT_METHOD"
[ "$DRY_RUN" = true ] && echo -e "${YELLOW}DRY RUN MODE${NC}"

# Function to check HA connectivity
check_ha_connectivity() {
  echo -e "${YELLOW}Checking Home Assistant connectivity...${NC}"

  # Try to reach HA API
  local response
  response=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Authorization: Bearer $HA_TOKEN" \
    -H "Content-Type: application/json" \
    "http://${HA_HOST}:8123/api/" 2>/dev/null || echo "000")

  if [ "$response" = "200" ]; then
    echo -e "${GREEN}Home Assistant is reachable and API is responding${NC}"
    return 0
  else
    echo -e "${RED}Cannot reach Home Assistant API (HTTP $response)${NC}"
    return 1
  fi
}

# Function to call HA API
call_ha_api() {
  local method=$1
  local endpoint=$2
  local data=${3:-}

  local url="http://${HA_HOST}:8123/api/${endpoint}"

  if [ -n "$data" ]; then
    curl -s -X "$method" \
      -H "Authorization: Bearer $HA_TOKEN" \
      -H "Content-Type: application/json" \
      -d "$data" \
      "$url"
  else
    curl -s -X "$method" \
      -H "Authorization: Bearer $HA_TOKEN" \
      -H "Content-Type: application/json" \
      "$url"
  fi
}

# Function to deploy blueprints via filesystem
deploy_blueprints_filesystem() {
  echo -e "${YELLOW}Deploying blueprints via filesystem...${NC}"

  local blueprint_src="$CONFIG_PATH/blueprints/automation"
  local blueprint_dst="$HA_CONFIG_DIR/blueprints/automation"

  if [ ! -d "$blueprint_src" ]; then
    echo -e "${YELLOW}No blueprints found in $blueprint_src${NC}"
    return 0
  fi

  if [ "$DRY_RUN" = true ]; then
    echo -e "${BLUE}Would copy: $blueprint_src -> $blueprint_dst${NC}"
    return 0
  fi

  # Create directory if it doesn't exist
  mkdir -p "$blueprint_dst"

  # Copy blueprint files
  for file in "$blueprint_src"/*.yaml; do
    if [ -f "$file" ]; then
      local filename=$(basename "$file")
      echo "  Copying blueprint: $filename"
      cp "$file" "$blueprint_dst/$filename"
    fi
  done

  echo -e "${GREEN}Blueprints deployed${NC}"
}

# Function to deploy dashboards via API
deploy_dashboards_api() {
  echo -e "${YELLOW}Deploying dashboards via API...${NC}"

  local dashboard_dir="$CONFIG_PATH/dashboards"

  if [ ! -d "$dashboard_dir" ]; then
    echo -e "${YELLOW}No dashboards found in $dashboard_dir${NC}"
    return 0
  fi

  # Check if Lovelace is in storage mode (required for API updates)
  local lovelace_mode
  lovelace_mode=$(call_ha_api GET "lovelace/dashboards" | grep -o '"mode":"[^"]*"' | head -1 | cut -d'"' -f4 || echo "unknown")

  if [ "$lovelace_mode" != "storage" ]; then
    echo -e "${YELLOW}Warning: Lovelace is in YAML mode. API-based dashboard deployment may not work.${NC}"
    echo -e "${YELLOW}Consider using filesystem deployment method instead.${NC}"
  fi

  for file in "$dashboard_dir"/*.yaml; do
    if [ -f "$file" ]; then
      local filename=$(basename "$file" .yaml)
      echo "  Processing dashboard: $filename"

      if [ "$DRY_RUN" = true ]; then
        echo -e "${BLUE}Would deploy dashboard: $filename${NC}"
        continue
      fi

      # Read dashboard content
      local dashboard_content
      dashboard_content=$(cat "$file")

      # Create or update dashboard via API
      # Note: This is a simplified approach. Full implementation would parse YAML and convert to JSON.
      echo -e "${YELLOW}    Note: Dashboard API deployment requires manual setup or advanced parsing${NC}"
      echo -e "${YELLOW}    Consider copying to /config/lovelace/ and using 'lovelace:' mode: storage${NC}"
    fi
  done

  echo -e "${GREEN}Dashboard processing complete${NC}"
}

# Function to deploy configuration helpers
deploy_config_helpers() {
  echo -e "${YELLOW}Checking configuration helpers...${NC}"

  local helpers_file="$CONFIG_PATH/configuration_helpers.yaml.example"

  if [ ! -f "$helpers_file" ]; then
    echo -e "${YELLOW}No configuration helpers found${NC}"
    return 0
  fi

  echo -e "${BLUE}Configuration helpers must be manually added to HA configuration.yaml${NC}"
  echo -e "${BLUE}See: $helpers_file${NC}"

  # Check if helpers already exist
  echo "  Checking for required input_boolean entities..."

  local entities=(
    "input_boolean.bed_presence_calibration_mode"
  )

  for entity in "${entities[@]}"; do
    local state
    state=$(call_ha_api GET "states/$entity" 2>/dev/null | grep -o '"state":"[^"]*"' | cut -d'"' -f4 || echo "not_found")

    if [ "$state" = "not_found" ]; then
      echo -e "    ${YELLOW}⚠ Missing: $entity${NC}"
    else
      echo -e "    ${GREEN}✓ Found: $entity (state: $state)${NC}"
    fi
  done
}

# Function to reload HA configuration
reload_ha_config() {
  echo -e "${YELLOW}Reloading Home Assistant configuration...${NC}"

  if [ "$DRY_RUN" = true ]; then
    echo -e "${BLUE}Would reload HA configuration${NC}"
    return 0
  fi

  # Reload automations
  echo "  Reloading automations..."
  local result
  result=$(call_ha_api POST "services/automation/reload" || echo "failed")

  if echo "$result" | grep -q "error"; then
    echo -e "${YELLOW}Warning: Could not reload automations${NC}"
  else
    echo -e "${GREEN}  Automations reloaded${NC}"
  fi

  # Reload scripts
  echo "  Reloading scripts..."
  result=$(call_ha_api POST "services/script/reload" || echo "failed")

  if echo "$result" | grep -q "error"; then
    echo -e "${YELLOW}Warning: Could not reload scripts${NC}"
  else
    echo -e "${GREEN}  Scripts reloaded${NC}"
  fi

  echo -e "${GREEN}Configuration reload complete${NC}"
}

# Function to verify ESPHome device integration
verify_esphome_device() {
  echo -e "${YELLOW}Verifying ESPHome device integration...${NC}"

  # Check if device entities exist
  local entities=(
    "binary_sensor.bed_occupied"
    "number.k_on_on_threshold_multiplier"
    "number.k_off_off_threshold_multiplier"
    "text_sensor.presence_state_reason"
  )

  local all_found=true

  for entity in "${entities[@]}"; do
    local state
    state=$(call_ha_api GET "states/$entity" 2>/dev/null | grep -o '"state":"[^"]*"' | cut -d'"' -f4 || echo "not_found")

    if [ "$state" = "not_found" ]; then
      echo -e "  ${RED}✗ Missing: $entity${NC}"
      all_found=false
    else
      echo -e "  ${GREEN}✓ Found: $entity (state: $state)${NC}"
    fi
  done

  if [ "$all_found" = true ]; then
    echo -e "${GREEN}All required entities found${NC}"
    return 0
  else
    echo -e "${YELLOW}Some entities are missing. Device may not be fully integrated yet.${NC}"
    return 1
  fi
}

# Main deployment logic
main() {
  echo "Starting Home Assistant configuration deployment..."

  # Step 1: Check connectivity
  if ! check_ha_connectivity; then
    echo -e "${RED}Cannot connect to Home Assistant${NC}"
    exit 1
  fi

  # Step 2: Deploy blueprints
  if [ "$DEPLOYMENT_METHOD" = "filesystem" ]; then
    deploy_blueprints_filesystem
  else
    echo -e "${YELLOW}Blueprint deployment via API not yet implemented${NC}"
    echo -e "${YELLOW}Use --method filesystem or manually copy blueprints${NC}"
  fi

  # Step 3: Deploy dashboards
  if [ "$DEPLOYMENT_METHOD" = "api" ]; then
    deploy_dashboards_api
  else
    echo -e "${YELLOW}Dashboard deployment via filesystem requires manual copy to $HA_CONFIG_DIR/lovelace/${NC}"
  fi

  # Step 4: Check configuration helpers
  deploy_config_helpers

  # Step 5: Reload configuration
  reload_ha_config

  # Step 6: Verify device integration
  echo ""
  if verify_esphome_device; then
    echo ""
    echo -e "${GREEN}=== Deployment Complete ===${NC}"
    echo "Status: Success ✓"
  else
    echo ""
    echo -e "${YELLOW}=== Deployment Complete with Warnings ===${NC}"
    echo "Status: Partial Success ⚠"
    echo ""
    echo "Next steps:"
    echo "1. Ensure ESPHome device is connected to Home Assistant"
    echo "2. Check Settings → Devices & Services → ESPHome"
    echo "3. Manually configure any missing helper entities"
  fi
}

# Run main function
main
