# Deployment Scripts

This directory contains automation scripts for GitOps-based deployment and testing.

## Scripts Overview

### 1. `deploy_esphome.sh`

Compiles and deploys ESPHome firmware to M5Stack device.

**Purpose**: Automate firmware updates via OTA or USB

**Usage**:
```bash
./deploy_esphome.sh \
  --device "192.168.1.100" \
  --password "ota-password" \
  --config "esphome/bed-presence-detector.yaml"
```

**Key Features**:
- Pre-deployment device reachability check
- Automatic firmware compilation
- Multiple deployment methods (OTA, USB, compile-only)
- Post-deployment verification
- Configurable timeout for device boot

**See**: `--help` for full options

---

### 2. `deploy_homeassistant.sh`

Deploys Home Assistant configuration (dashboards, blueprints, helpers).

**Purpose**: Sync HA config from repository to live instance

**Usage**:
```bash
./deploy_homeassistant.sh \
  --ha-host "192.168.1.50" \
  --ha-token "your-token" \
  --config-path "homeassistant/"
```

**Key Features**:
- API connectivity verification
- Entity existence checks
- Configuration reload (automations, scripts)
- Blueprint deployment
- Dry-run mode for testing

**See**: `--help` for full options

---

### 3. `run_e2e_tests.sh`

Executes end-to-end integration tests against live hardware.

**Purpose**: Validate full system integration with automated tests

**Usage**:
```bash
./run_e2e_tests.sh \
  --ha-url "ws://192.168.1.50:8123/api/websocket" \
  --ha-token "your-token" \
  --device-name "bed_presence_detector"
```

**Key Features**:
- Pre-flight checks (HA connectivity, device integration)
- Automatic dependency installation
- JUnit XML report generation
- Verbose mode for debugging
- Test summary reporting

**See**: `--help` for full options

---

## Quick Start

### Prerequisites

- Python 3.11+
- ESPHome CLI: `pip install esphome`
- pytest: `pip install pytest pytest-asyncio`
- Network access to M5Stack device and Home Assistant

### One-Time Setup

1. **Configure secrets** in repository:
   - See `.github/workflows/preview_deployment.yml` for required secrets

2. **Test connectivity**:
   ```bash
   # Test device
   ping 192.168.1.100

   # Test Home Assistant API
   curl -H "Authorization: Bearer YOUR_TOKEN" \
     http://192.168.1.50:8123/api/
   ```

### Manual Deployment Example

```bash
# 1. Deploy firmware
./scripts/deploy_esphome.sh \
  --device "192.168.1.100" \
  --password "ota-password" \
  --config "esphome/bed-presence-detector.yaml"

# 2. Deploy HA configuration
./scripts/deploy_homeassistant.sh \
  --ha-host "192.168.1.50" \
  --ha-token "your-token" \
  --config-path "homeassistant/"

# 3. Run E2E tests
./scripts/run_e2e_tests.sh \
  --ha-url "ws://192.168.1.50:8123/api/websocket" \
  --ha-token "your-token"
```

## Automated Usage (GitHub Actions)

These scripts are automatically called by `.github/workflows/preview_deployment.yml` on every pull request.

**Workflow sequence**:
1. Compile firmware (GitHub-hosted runner)
2. Run unit tests (GitHub-hosted runner)
3. Deploy to hardware (self-hosted runner) ← **Uses these scripts**
4. Run E2E tests (self-hosted runner) ← **Uses these scripts**
5. Report results in PR comment

See `docs/gitops-deployment-guide.md` for full workflow documentation.

## Script Dependencies

### `deploy_esphome.sh`
- ESPHome CLI
- Network access to device
- (Optional) USB serial access for USB deployment

### `deploy_homeassistant.sh`
- curl
- jq (optional, for JSON parsing)
- Network access to Home Assistant
- Valid long-lived access token

### `run_e2e_tests.sh`
- Python 3.11+
- pytest, pytest-asyncio
- homeassistant-api (from `tests/e2e/requirements.txt`)
- Network access to Home Assistant

## Exit Codes

All scripts follow standard Unix exit code conventions:

- `0`: Success
- `1`: Error (see stderr for details)
- `2+`: Script-specific errors

Scripts are designed to:
- Exit immediately on errors (`set -e`)
- Provide colored output for readability
- Log detailed information for debugging
- Support non-interactive CI/CD execution

## Environment Variables

Scripts can be configured via environment variables for CI/CD:

### `deploy_esphome.sh`
```bash
export ESPHOME_DEVICE_HOST="192.168.1.100"
export ESPHOME_OTA_PASSWORD="secret"
./deploy_esphome.sh --config "esphome/bed-presence-detector.yaml"
```

### `deploy_homeassistant.sh`
```bash
export HA_HOST="192.168.1.50"
export HA_TOKEN="secret-token"
./deploy_homeassistant.sh --config-path "homeassistant/"
```

### `run_e2e_tests.sh`
```bash
export HA_URL="ws://192.168.1.50:8123/api/websocket"
export HA_TOKEN="secret-token"
./run_e2e_tests.sh
```

Command-line arguments take precedence over environment variables.

## Troubleshooting

### Common Issues

#### "Device not reachable"
```bash
# Check network connectivity
ping 192.168.1.100

# Check if ESPHome API is responding
nc -zv 192.168.1.100 6053
```

#### "Home Assistant API error"
```bash
# Verify token
curl -s -H "Authorization: Bearer YOUR_TOKEN" \
  http://192.168.1.50:8123/api/ | jq

# Check HA logs
# Settings → System → Logs in Home Assistant
```

#### "Tests failed"
```bash
# Run tests with verbose output
./run_e2e_tests.sh \
  --ha-url "ws://192.168.1.50:8123/api/websocket" \
  --ha-token "your-token" \
  --verbose

# Check test logs
cat tests/e2e/*.log
```

### Debug Mode

All scripts support verbose output:

```bash
# Add bash debug flag
bash -x ./deploy_esphome.sh [args...]

# Or modify script temporarily
# Change: set -e
# To: set -ex
```

## Contributing

When modifying these scripts:

1. Maintain backwards compatibility
2. Update `--help` output
3. Add error handling for new failure modes
4. Test both manual and CI/CD execution
5. Update this README with any new options

## Additional Documentation

- **Setup**: `docs/self-hosted-runner-setup.md`
- **Workflow**: `docs/gitops-deployment-guide.md`
- **Testing**: `tests/e2e/README.md` (if exists)

## License

Same as parent repository (see root LICENSE file).
