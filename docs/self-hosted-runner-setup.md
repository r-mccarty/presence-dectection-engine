# Self-Hosted GitHub Actions Runner Setup

This guide walks through setting up a self-hosted GitHub Actions runner for automated preview deployments and E2E testing with physical hardware.

## Overview

The preview deployment workflow requires a self-hosted runner because it needs:

1. **Network access** to the M5Stack device (for OTA firmware updates)
2. **Network access** to Home Assistant instance (for configuration deployment)
3. **Ability to run E2E tests** against live hardware
4. **USB access** (optional, for direct flashing)

A self-hosted runner on the same local network as your devices provides this access.

## Prerequisites

### Hardware Requirements

- **Runner Host**: Linux machine, Raspberry Pi, or VM on the same network as:
  - M5Stack device running ESPHome
  - Home Assistant instance
- **Minimum specs**: 2GB RAM, 2 CPU cores, 20GB storage
- **Network**: Wired Ethernet recommended for stability

### Software Requirements

- Linux (Ubuntu 20.04+ or Debian 11+ recommended)
- Python 3.11+
- Git
- curl
- systemd (for auto-start)

### GitHub Repository Access

- Repository admin or maintainer permissions
- Ability to add self-hosted runners in repository settings

## Installation Steps

### Step 1: Prepare the Runner Host

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y \
  git \
  curl \
  python3 \
  python3-pip \
  python3-venv \
  build-essential \
  libssl-dev \
  libffi-dev

# Install ESPHome and dependencies
pip3 install esphome platformio

# Add to PATH (if needed)
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Verify installations
python3 --version
esphome version
platformio --version
```

### Step 2: Download and Configure GitHub Actions Runner

Navigate to your repository on GitHub:

1. Go to **Settings** → **Actions** → **Runners**
2. Click **New self-hosted runner**
3. Select **Linux** as the OS
4. Follow the instructions to download and configure the runner

**Example commands** (replace with actual URLs from GitHub):

```bash
# Create a directory for the runner
mkdir -p ~/actions-runner && cd ~/actions-runner

# Download the runner package
curl -o actions-runner-linux-x64-2.311.0.tar.gz -L \
  https://github.com/actions/runner/releases/download/v2.311.0/actions-runner-linux-x64-2.311.0.tar.gz

# Extract the installer
tar xzf ./actions-runner-linux-x64-2.311.0.tar.gz

# Configure the runner
./config.sh --url https://github.com/YOUR_ORG/bed-presence-sensor \
  --token YOUR_REGISTRATION_TOKEN \
  --name "bed-presence-runner" \
  --labels "self-hosted,linux,esphome,homeassistant" \
  --work _work
```

**Important configuration options:**

- `--name`: Give it a descriptive name like "bed-presence-runner"
- `--labels`: Add labels to target this runner specifically
  - Recommended: `self-hosted,linux,esphome,homeassistant,hardware`
- `--work`: Directory for job workspaces

### Step 3: Install Runner as a Service

This ensures the runner starts automatically on boot:

```bash
cd ~/actions-runner

# Install the service
sudo ./svc.sh install

# Start the service
sudo ./svc.sh start

# Check status
sudo ./svc.sh status
```

To view logs:

```bash
# View service logs
sudo journalctl -u actions.runner.* -f

# Or check the runner logs directly
tail -f ~/actions-runner/_diag/Runner_*.log
```

### Step 4: Configure Network Access

#### Allow Runner to Access M5Stack Device

Verify the runner can reach the M5Stack:

```bash
# Test ping
ping -c 3 <M5STACK_IP_ADDRESS>

# Test ESPHome API port
nc -zv <M5STACK_IP_ADDRESS> 6053

# Test if device is responding
esphome logs <M5STACK_IP_ADDRESS> --device <M5STACK_IP_ADDRESS>
```

If you encounter issues:

1. Check firewall rules on runner host
2. Ensure M5Stack and runner are on same VLAN/subnet
3. Verify M5Stack has a static IP or DHCP reservation

#### Allow Runner to Access Home Assistant

```bash
# Test ping
ping -c 3 <HA_IP_ADDRESS>

# Test HA API
curl -s -H "Authorization: Bearer YOUR_TOKEN" \
  http://<HA_IP_ADDRESS>:8123/api/ | jq
```

If you encounter issues:

1. Check Home Assistant firewall/network settings
2. Verify the long-lived access token is valid
3. Ensure runner is on allowed network in HA configuration

### Step 5: Configure Repository Secrets

Add the following secrets to your GitHub repository:

Go to **Settings** → **Secrets and variables** → **Actions** → **New repository secret**

#### Required Secrets

| Secret Name | Description | Example |
|-------------|-------------|---------|
| `PREVIEW_WIFI_SSID` | WiFi SSID for M5Stack | `HomeNetwork` |
| `PREVIEW_WIFI_PASSWORD` | WiFi password | `SecurePassword123` |
| `PREVIEW_API_KEY` | ESPHome API encryption key | Generate with `esphome` wizard |
| `PREVIEW_OTA_PASSWORD` | ESPHome OTA password | Generate with `esphome` wizard |
| `ESPHOME_DEVICE_HOST` | M5Stack IP address or hostname | `192.168.1.100` or `bed-sensor.local` |
| `HA_HOST` | Home Assistant IP or hostname | `192.168.1.50` or `homeassistant.local` |
| `HA_TOKEN` | HA long-lived access token | Create in HA Profile settings |

#### Generate ESPHome Keys

```bash
# Generate API encryption key
python3 -c "import secrets; print(secrets.token_urlsafe(32))"

# Generate OTA password
python3 -c "import secrets; print(secrets.token_urlsafe(16))"
```

#### Create Home Assistant Token

1. In Home Assistant, go to your **Profile** (bottom left)
2. Scroll to **Long-Lived Access Tokens**
3. Click **Create Token**
4. Name it "GitHub Actions Runner"
5. Copy the token (you can only see it once!)

### Step 6: Test the Runner

Create a simple test workflow to verify the runner works:

**`.github/workflows/test-runner.yml`:**

```yaml
name: Test Self-Hosted Runner

on:
  workflow_dispatch:

jobs:
  test:
    runs-on: self-hosted
    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Test Python
        run: python3 --version

      - name: Test ESPHome
        run: esphome version

      - name: Test network access to M5Stack
        run: ping -c 3 ${{ secrets.ESPHOME_DEVICE_HOST }}

      - name: Test network access to Home Assistant
        run: ping -c 3 ${{ secrets.HA_HOST }}

      - name: Test HA API
        run: |
          curl -s -H "Authorization: Bearer ${{ secrets.HA_TOKEN }}" \
            http://${{ secrets.HA_HOST }}:8123/api/ | jq
```

Run this workflow manually from the Actions tab to verify everything works.

## Runner Maintenance

### Updating the Runner

GitHub periodically releases new runner versions:

```bash
cd ~/actions-runner

# Stop the service
sudo ./svc.sh stop

# Download and extract new version
# (Get URL from GitHub Settings → Runners → New self-hosted runner)

# Restart the service
sudo ./svc.sh start
```

### Monitoring Runner Health

```bash
# Check if runner is online
curl -H "Authorization: token YOUR_GITHUB_PAT" \
  https://api.github.com/repos/YOUR_ORG/bed-presence-sensor/actions/runners

# Check disk space
df -h ~/actions-runner/_work

# Check system resources
htop
```

### Cleaning Up Old Workspaces

The runner accumulates build artifacts over time:

```bash
# Clean old workspaces (older than 7 days)
find ~/actions-runner/_work -type d -mtime +7 -exec rm -rf {} +

# Clean ESPHome cache
rm -rf ~/.esphome/build/*
rm -rf ~/.platformio/.cache/*
```

Consider adding a cron job:

```bash
# Edit crontab
crontab -e

# Add weekly cleanup (runs Sunday at 2 AM)
0 2 * * 0 find ~/actions-runner/_work -type d -mtime +7 -exec rm -rf {} +
```

## Security Considerations

### Runner Isolation

- Run the runner as a **non-root user** with limited permissions
- Use a dedicated user account (e.g., `github-runner`)
- Limit network access using firewall rules

### Secret Management

- **Never log secrets** in workflow outputs
- Use GitHub encrypted secrets for all sensitive data
- Rotate tokens and passwords regularly (every 90 days recommended)

### Network Security

- Place runner on isolated VLAN if possible
- Use firewall rules to restrict runner access to only necessary hosts
- Enable mDNS/DNS for hostname resolution (avoid hardcoding IPs)

### Physical Security

- Secure physical access to runner host
- Enable full disk encryption if running on portable hardware
- Use UPS to prevent data corruption during power loss

## Troubleshooting

### Runner Not Appearing in GitHub

**Symptoms**: Runner configured but not showing as "Idle" in GitHub

**Solutions**:

1. Check service status: `sudo ./svc.sh status`
2. Check logs: `tail -f ~/actions-runner/_diag/Runner_*.log`
3. Verify network connectivity to GitHub: `ping github.com`
4. Re-register the runner: `./config.sh remove` then reconfigure

### Jobs Stuck in "Queued" State

**Symptoms**: Workflows wait indefinitely for runner

**Solutions**:

1. Verify runner is online in Settings → Runners
2. Check runner labels match job requirements
3. Ensure runner has capacity (not running max concurrent jobs)
4. Check runner logs for errors

### ESPHome Deployment Fails

**Symptoms**: `deploy_esphome.sh` fails with connection errors

**Solutions**:

1. Verify M5Stack is on network: `ping <DEVICE_IP>`
2. Check ESPHome API port: `nc -zv <DEVICE_IP> 6053`
3. Verify OTA password matches device configuration
4. Check ESPHome device logs in Home Assistant
5. Try manual OTA: `esphome upload bed-presence-detector.yaml --device <IP>`

### Home Assistant API Fails

**Symptoms**: `deploy_homeassistant.sh` fails with 401/403 errors

**Solutions**:

1. Verify token is valid: Try API call manually with curl
2. Check token hasn't expired (HA tokens don't expire by default, but can be revoked)
3. Verify HA is accessible: `curl http://<HA_IP>:8123`
4. Check HA logs: Settings → System → Logs
5. Ensure HA authentication is not in "trusted networks only" mode

### E2E Tests Fail

**Symptoms**: Tests fail with connection or entity errors

**Solutions**:

1. Verify all entities exist in HA: Check Settings → Devices
2. Run tests manually on runner host to see detailed errors
3. Check ESPHome device is connected to HA
4. Verify test dependencies: `pip install -r tests/e2e/requirements.txt`
5. Check entity names match Phase 1 expectations

### Disk Space Issues

**Symptoms**: Runner fails with "no space left" errors

**Solutions**:

```bash
# Check disk usage
df -h

# Clean workspace
rm -rf ~/actions-runner/_work/*

# Clean ESPHome cache
rm -rf ~/.esphome/build/*
rm -rf ~/.platformio/.cache/*

# Clean pip cache
pip cache purge
```

## Advanced Configuration

### Multiple Runners

For redundancy or parallel testing:

1. Set up additional runner hosts
2. Use different names but same labels
3. GitHub will distribute jobs across available runners

### USB Flashing Support

If runner has USB access to M5Stack:

```bash
# Install USB serial drivers
sudo apt install -y python3-serial

# Add runner user to dialout group
sudo usermod -a -G dialout github-runner

# Logout and login for group change to take effect

# Test USB detection
ls -l /dev/ttyUSB*

# Update workflow to use USB deployment method
# (See deploy_esphome.sh --method usb --usb-port /dev/ttyUSB0)
```

### Custom Python Environment

Use a virtual environment for better isolation:

```bash
# Create venv
python3 -m venv ~/runner-env

# Activate and install dependencies
source ~/runner-env/bin/activate
pip install esphome platformio pytest

# Update service to use venv
# Edit ~/actions-runner/.env
echo 'PATH='$HOME'/runner-env/bin:$PATH' >> ~/actions-runner/.env

# Restart service
sudo ./svc.sh restart
```

## Additional Resources

- [GitHub Actions Self-Hosted Runners Documentation](https://docs.github.com/en/actions/hosting-your-own-runners)
- [ESPHome OTA Updates](https://esphome.io/components/ota.html)
- [Home Assistant API](https://developers.home-assistant.io/docs/api/rest/)
- [pytest Documentation](https://docs.pytest.org/)

## Summary Checklist

Before your first preview deployment, verify:

- [ ] Runner is online and showing "Idle" in GitHub
- [ ] All repository secrets are configured
- [ ] Runner can ping M5Stack device
- [ ] Runner can ping Home Assistant
- [ ] ESPHome CLI works on runner: `esphome version`
- [ ] HA API is accessible: `curl -H "Authorization: Bearer $TOKEN" http://<HA_IP>:8123/api/`
- [ ] Test workflow completes successfully
- [ ] Scripts are executable: `ls -l scripts/*.sh`
- [ ] Python dependencies installed: `pip list | grep -E "esphome|pytest|homeassistant"`

Once all items are checked, you're ready to create a PR and trigger your first preview deployment!
