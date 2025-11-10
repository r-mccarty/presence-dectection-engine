# Ubuntu Home Assistant Node - Setup Documentation

## Node Overview

**Hardware**: N100 Mini PC
**OS**: Ubuntu 24.04 LTS (Linux kernel 6.14.0-34-generic)
**Hostname**: `home-assistant-node`
**Primary User**: `ryan`
**Cloudflare SSH Hostname**: `ssh.hardwareos.com`
**Location**: Local LAN

## Network Configuration

### Local Network
- **LAN IP**: Private network (192.168.x.x range)
- **Network**: Behind router (not directly accessible from internet)

### Public Access via Cloudflare Tunnel

The node uses Cloudflare Tunnel for secure external access:
- SSH: `ssh.hardwareos.com` → `localhost:22`
- Home Assistant: see `/etc/cloudflared/config.yml` for the current hostname → `localhost:8123`

> **Note**: Cloudflare ingress rules live in `/etc/cloudflared/config.yml` on the node. Update that file (and restart the `cloudflared` service) if hostnames change.

## SSH Access Configuration

### Cloudflare Tunnel Setup

The tunnel is configured and running as a system service:

```bash
# Check tunnel status
sudo systemctl status cloudflared

# View tunnel configuration
cat /etc/cloudflared/config.yml
```

**Tunnel ingress configuration example**:
```yaml
ingress:
  - hostname: ha.hardwareos.com      # See /etc/cloudflared/config.yml for the authoritative HA hostname
    service: http://localhost:8123

  - hostname: ssh.hardwareos.com
    service: ssh://localhost:22

  - service: http_status:404
```

### Connecting via SSH from Codespaces/Remote

You reach the node through Cloudflare Tunnel at `ssh.hardwareos.com`. Cloudflare Access for this hostname currently allows direct tunneling (no Cloudflare login prompt), so you only need working SSH credentials for the `claude-temp` user.

#### Prerequisites

1. **cloudflared CLI** – provides the SOCKS tunnel that proxies SSH through Cloudflare.  
   - *Codespaces*: installed automatically by `.devcontainer/devcontainer.json` (run `cloudflared --version` to verify, currently `2025.11.1`).  
   - *Other environments*: download the static binary and install it on your `$PATH`.
     ```bash
     curl -fsSLo /tmp/cloudflared https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64
     sudo install -m 0755 /tmp/cloudflared /usr/local/bin/cloudflared
     rm /tmp/cloudflared
     ```
2. **SSH config entry** – ensures every `ssh ubuntu-node` command automatically runs through the Cloudflare proxy.
   ```
   Host ubuntu-node
     HostName ssh.hardwareos.com
     User claude-temp
     IdentityFile ~/.ssh/claude-temp
     ProxyCommand cloudflared access ssh --hostname ssh.hardwareos.com
     StrictHostKeyChecking no
     UserKnownHostsFile /dev/null
   ```
   > **Codespaces automation**: the devcontainer creates this block (with the `IdentityFile` line) as soon as the container comes up.
3. **Authorized SSH key** – the `claude-temp` user only accepts key-based auth. You can either provision a reusable key via a Codespaces secret (recommended) or generate a one-off key per Codespace.

#### Option A: Reusable key via Codespaces secret

1. Generate an ed25519 key once on a trusted machine:
   ```bash
   ssh-keygen -t ed25519 -f ~/.ssh/claude-temp -C "claude-temp reusable"
   ```
2. Add the **public** half to `/home/claude-temp/.ssh/authorized_keys` on the ubuntu-node (ask the node owner if you do not have access yet).
3. Store the **private** key as a GitHub Codespaces secret named `SSH_CLAUDE_TEMP_KEY`.  
   - GitHub → Settings → Codespaces → Secrets → New secret → Name `SSH_CLAUDE_TEMP_KEY`, Value = full private key contents (including the `-----BEGIN...` lines).  
   - Secrets are scoped to **your GitHub account**; other contributors cannot read or use them, even though this repository is public.
4. On the next Codespace rebuild, the devcontainer will detect `SSH_CLAUDE_TEMP_KEY`, write it to `~/.ssh/claude-temp`, and protect it with `0600` permissions automatically. Verify with:
   ```bash
   ls -l ~/.ssh/claude-temp
   ```
   > **Warning**: Generate the private key inside the Codespace (or another trusted local machine) and copy **only the public half** to the ubuntu-node. Generating the key on the node and pasting the public portion into a Codespaces secret reverses the trust boundary—the Codespace would then hold a public key that cannot authenticate anywhere.

#### Recommended developer ↔ admin workflow

1. **Developer (Codespace)**: Run `ssh-keygen -t ed25519 -f ~/.ssh/claude-temp -C "claude-temp@$(hostname)"` and copy the output of `cat ~/.ssh/claude-temp.pub`.
2. **Admin (ubuntu-node)**: Append that public key to `/home/claude-temp/.ssh/authorized_keys`, making sure the directory exists with `700` perms and the file with `600` perms owned by `claude-temp:claude-temp`.
3. **Developer**: Store the private key contents from `~/.ssh/claude-temp` as the `SSH_CLAUDE_TEMP_KEY` Codespaces secret so future containers recreate the same identity automatically.
4. **Developer**: Rebuild or restart the Codespace (or run `devcontainer rebuild`) so the secret is materialized at `~/.ssh/claude-temp`, then run `ssh -vv ubuntu-node true` to confirm Cloudflare tunnel access.

> **Common pitfall**: Some contributors mistakenly run `ssh-keygen` on the ubuntu-node and paste the resulting *public* key into a Codespaces secret. This cannot work because SSH needs the **private** key on the client (Codespace) and the **public** key on the server (ubuntu-node). Always keep the private key client-side and only share the matching public key with the admin.

> **Security note**: If you revoke access to the ubuntu-node, delete the secret and remove the corresponding public key from `/home/claude-temp/.ssh/authorized_keys`.

#### Option B: One-off key per Codespace

If you prefer short-lived credentials, generate the key inside the Codespace:
```bash
ssh-keygen -t ed25519 -f ~/.ssh/claude-temp -C "claude-temp@$(hostname)"
cat ~/.ssh/claude-temp.pub
```
Send the `.pub` contents to the node owner (`ryan`) or, if you already have sudo on the node from a previous session, add it yourself:
```bash
sudo install -d -m 0700 -o claude-temp -g claude-temp /home/claude-temp/.ssh
sudo tee -a /home/claude-temp/.ssh/authorized_keys <<<"ssh-ed25519 AAAA... generated-on-codespace"
sudo chown claude-temp:claude-temp /home/claude-temp/.ssh/authorized_keys
sudo chmod 600 /home/claude-temp/.ssh/authorized_keys
```

#### Connection workflow

```bash
# Quick connectivity test
ssh -vv ubuntu-node true

# Start an interactive shell
ssh ubuntu-node
```

If authentication fails, double-check that the private key path matches the `IdentityFile` setting and that the matching public key exists in `/home/claude-temp/.ssh/authorized_keys`. Because the Cloudflare tunnel does not prompt for Access authentication, any failure at this stage is almost always due to missing/incorrect SSH keys.

### SSH Port Forwarding for Home Assistant Access

**Problem**: When working in GitHub Codespaces, you cannot directly access the Home Assistant web interface at `192.168.0.148:8123` because:
- Codespace runs on GitHub's cloud infrastructure
- HA is on your local network (192.168.0.148)
- Different networks cannot communicate directly

**Solution**: Use SSH port forwarding to create a tunnel through ubuntu-node.

**Accessing HA Web UI from Codespace Browser**:

```bash
# In Codespace terminal
ssh -L 8123:192.168.0.148:8123 ubuntu-node
```

**What this does**:
- Creates a tunnel: `localhost:8123` (Codespace) → `192.168.0.148:8123` (HA on ubuntu-node's network)
- Keep the SSH session open (don't close the terminal)

**Then open in Codespace browser**: `http://localhost:8123`

**SSH Tunnel Options**:

```bash
# Basic tunnel (blocks terminal)
ssh -L 8123:192.168.0.148:8123 ubuntu-node

# Run in background (returns terminal to you)
ssh -fN -L 8123:192.168.0.148:8123 ubuntu-node

# With keep-alive (prevents timeout)
ssh -o ServerAliveInterval=60 -L 8123:192.168.0.148:8123 ubuntu-node
```

**Stopping background tunnel**:
```bash
# Find the SSH process
ps aux | grep "ssh.*8123"

# Kill it
kill <process_id>
```

**Port Forwarding for Scripts**:

If running Python scripts in Codespace that need HA API access:

```bash
# Terminal 1: Create tunnel
ssh -L 8123:192.168.0.148:8123 ubuntu-node

# Terminal 2: Run script
cd /workspaces/bed-presence-sensor
python3 scripts/collect_baseline.py
# Script will connect to localhost:8123, which tunnels to HA
```

**Note**: It's usually easier to run scripts directly on ubuntu-node instead of using tunnels from Codespace.

## User Accounts

### Development User Setup

**Username**: Varies per session (e.g., `claude-temp` for temporary access)
**Groups**: sudo, docker, dialout
**Sudo Access**: Full passwordless sudo (NOPASSWD: ALL) - for development only
**Home Directory**: `/home/your-username`

**Purpose**: Temporary user for development and testing. Should be removed after session.

**SSH Key Setup**:
- Generate SSH key pair on development machine
- Add public key to `~/.ssh/authorized_keys` on node
- Store private key securely

**Cleanup instructions** (when session ends):
```bash
sudo deluser your-username
sudo rm -rf /home/your-username
# Remove SSH public key from authorized_keys
```

### Primary User

**Username**: System owner
**Home Directory**: Varies per installation
**Home Assistant Config**: `/opt/homeassistant/config`

## Home Assistant Setup

### Docker Container

Home Assistant runs in a Docker container:

```bash
# Check container status
docker ps | grep homeassistant

# View logs
docker logs homeassistant

# Restart container
docker restart homeassistant
```

### Configuration Directory

**Location**: `/opt/homeassistant/config`
**Permissions**: Group writable by `ryan` group
**Access**: Members of `ryan` group can read/write

```bash
# Access config directory
cd /opt/homeassistant/config

# View configuration.yaml
cat /opt/homeassistant/config/configuration.yaml
```

### Web Access

- **Local**: http://YOUR_NODE_IP:8123 (find IP via `ip addr` or router DHCP list)
- **Public**: `https://<cf-ha-hostname>` (via Cloudflare Tunnel; see `/etc/cloudflared/config.yml` for the active value)

### API Access

**WebSocket API**: `ws://YOUR_NODE_IP:8123/api/websocket` (local network)
**Long-Lived Access Tokens**: Create via Profile → Security → Long-Lived Access Tokens

**Note**: Store tokens securely and never commit them to git.

## ESPHome Development Environment

### Python Virtual Environment

**Location**: `/home/your-username/esphome-venv` (or wherever you set up the venv)
**Python Version**: 3.12.3
**ESPHome Version**: 2025.10.4

**Activate**:
```bash
source ~/esphome-venv/bin/activate
```

### Repository Location

**Path**: Clone the repository to your preferred location (e.g., `~/bed-presence-sensor`)

**Key directories**:
- `esphome/` - Firmware source and configuration
- `esphome/custom_components/bed_presence_engine/` - Custom C++ component
- `esphome/secrets.yaml` - WiFi credentials (gitignored)
- `tests/e2e/` - Integration tests

### ESPHome Commands

```bash
# Navigate to project
cd ~/bed-presence-sensor/esphome

# Activate virtual environment
source ~/esphome-venv/bin/activate

# Compile firmware
esphome compile bed-presence-detector.yaml

# Upload to device
esphome upload bed-presence-detector.yaml --device /dev/ttyACM0

# View logs
esphome logs bed-presence-detector.yaml --device /dev/ttyACM0

# Or compile and upload in one command
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

## M5Stack Device Configuration

### Hardware Details

**Device**: M5Stack Basic
**Board**: m5stack-core-esp32
**Chip**: ESP32-D0WDQ6-V3 (revision v3.1)
**MAC Address**: Unique per device (viewable in ESPHome logs)
**Flash Size**: 16MB

### USB Connection

**Serial Device**: `/dev/ttyACM0` (may vary - check with `ls /dev/tty*`)
**USB Vendor ID**: 1a86 (QinHeng Electronics)
**USB Product ID**: 55d4 (USB Single Serial)
**Baud Rate**: 115200

**Check connection**:
```bash
ls -la /dev/ttyACM*
lsusb | grep 1a86
```

### Network Configuration

**WiFi Network**: Configured in `secrets.yaml` (gitignored)
**IP Address**: Assigned by DHCP (viewable in Home Assistant or router)
**Hostname**: bed-presence-detector.local
**mDNS**: Yes (ESPHome API on port 6053)

**Note**: WiFi credentials are stored locally in `esphome/secrets.yaml` and should never be committed to git.

### LD2410 Sensor Configuration

**Connection**: UART
**TX Pin**: GPIO16
**RX Pin**: GPIO17
**Baud Rate**: 256000

**Sensor Details**:
- **Model**: LD2410 mmWave Radar
- **Purpose**: Presence detection via still energy readings
- **Key Sensor**: `sensor.ld2410_still_energy`

## Home Assistant Entities

### Binary Sensors
- `binary_sensor.bed_occupied` - Phase 1 presence detection
- `binary_sensor.ld2410_presence` - Raw LD2410 presence
- `binary_sensor.ld2410_moving_target` - Moving detection
- `binary_sensor.ld2410_still_target` - Still detection

### Sensors
- `sensor.ld2410_still_energy` - **Primary input for presence engine**
- `sensor.ld2410_moving_energy` - Moving energy reading
- `sensor.ld2410_detection_distance` - Distance to detected target
- `sensor.ld2410_still_distance` - Still target distance
- `sensor.ld2410_moving_distance` - Moving target distance

### Number Entities (Tunable Thresholds)
- `number.k_on_on_threshold_multiplier` - Turn ON when z > k_on (default: 4.0)
- `number.k_off_off_threshold_multiplier` - Turn OFF when z < k_off (default: 2.0)

### Text Sensors
- `text_sensor.presence_state_reason` - Debug info showing z-scores
- `text_sensor.ip_address` - Device IP address

## Secrets Configuration

**File**: `esphome/secrets.yaml` (gitignored - never commit this file!)

**Template** (use `esphome/secrets.yaml.example` as reference):
```yaml
wifi_ssid: "Your_WiFi_Network"
wifi_password: "your_wifi_password"
ap_password: "fallback-hotspot-password"  # Min 8 characters
api_encryption_key: "generate_with_command_below"
ota_password: "your_ota_password"
```

**Generate encryption key**:
```bash
python3 -c "import secrets; import base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"
```

**Security Notes**:
- This file contains sensitive credentials and is gitignored
- Never commit `secrets.yaml` to version control
- Store credentials securely (password manager recommended)
- Use strong, unique passwords for `ap_password` and `ota_password`
- The `api_encryption_key` must match what Home Assistant expects

## Development Workflow

**⚠️ CRITICAL**: This project uses a **two-location workflow** (Codespace for development, Ubuntu-node for flashing). It's essential to understand where code changes should be made and how to sync them properly.

---

## Codespace ↔ Ubuntu-node Workflow

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                       DEVELOPMENT LOCATIONS                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────────────────┐              ┌──────────────────────┐   │
│  │   GitHub Codespace   │              │    Ubuntu Node       │   │
│  │  ─────────────────   │              │  ────────────────    │   │
│  │  • Code editing      │◄────git─────►│  • Firmware flash   │   │
│  │  • Git operations    │              │  • Device testing   │   │
│  │  • Documentation     │              │  • USB connection   │   │
│  │  • NO device access  │              │  • HA integration   │   │
│  └──────────────────────┘              └──────────────────────┘   │
│         ▲                                         │                 │
│         │                                         │                 │
│         └───────── git push/pull ─────────────────┘                │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘

                              ▼
                    ┌──────────────────┐
                    │   M5Stack Device  │
                    │  ───────────────  │
                    │  USB connected to │
                    │  Ubuntu Node only │
                    └──────────────────┘
```

### Key Principles

1. **Source of Truth**: Git repository (GitHub)
2. **Code Development**: GitHub Codespace
3. **Firmware Compilation & Flashing**: Ubuntu-node (has physical device)
4. **Syncing Method**: Git (push from Codespace → pull on ubuntu-node)

### Common Pitfall ⚠️

**NEVER** edit code directly on ubuntu-node without committing to git. This leads to:
- Codespace and ubuntu-node having different versions
- Confusion about which code is "correct"
- Risk of flashing outdated firmware
- Loss of work when files get out of sync

---

## Workflow 1: Making Code Changes

**When to use**: Editing C++ code, YAML configs, or documentation

### Step 1: Edit in Codespace

```bash
# In GitHub Codespace (browser or VS Code)
cd /workspaces/bed-presence-sensor

# Edit files (example: updating baseline calibration)
nano esphome/custom_components/bed_presence_engine/bed_presence.h

# Or edit YAML configurations
nano esphome/bed-presence-detector.yaml
```

### Step 2: Test Compilation in Codespace (Optional but Recommended)

```bash
cd /workspaces/bed-presence-sensor/esphome

# Compile to verify syntax (won't actually flash)
esphome compile bed-presence-detector.yaml

# Run C++ unit tests
platformio test -e native
```

**Why this matters**: Catch syntax errors BEFORE syncing to ubuntu-node.

### Step 3: Commit to Git

```bash
cd /workspaces/bed-presence-sensor

# Stage your changes
git add esphome/custom_components/bed_presence_engine/bed_presence.h
git add esphome/bed-presence-detector.yaml

# Commit with descriptive message
git commit -m "Update baseline calibration with real values

- Set μ=6.3% (mean still energy, empty bed)
- Set σ=2.6% (std dev still energy)
- Collected 2025-11-05 with 30 samples

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"

# Push to GitHub
git push origin main
```

### Step 4: Sync to Ubuntu-node

```bash
# SSH into ubuntu-node
ssh ubuntu-node

# Navigate to repository
cd ~/bed-presence-sensor

# Pull latest changes from GitHub
git pull origin main

# Verify the changes are present
git log -1  # Check latest commit
```

### Step 5: Flash Firmware

```bash
# On ubuntu-node (still connected via SSH)
cd ~/bed-presence-sensor/esphome
source ~/esphome-venv/bin/activate

# Flash device (compiles + uploads)
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

**✅ Result**: Firmware on device matches code in Codespace and git

---

## Workflow 2: Quick Iteration (Testing on Device)

**When to use**: Rapid testing of small changes (threshold tweaks, debugging)

**⚠️ DANGER ZONE**: This workflow bypasses git and can cause sync issues. Use sparingly and commit immediately after.

### Option A: Edit on Ubuntu-node, Commit, Sync Back (RECOMMENDED)

```bash
# 1. SSH into ubuntu-node
ssh ubuntu-node
cd ~/bed-presence-sensor

# 2. Make quick change
nano esphome/custom_components/bed_presence_engine/bed_presence.h

# 3. Flash immediately
cd esphome
source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0

# 4. IMMEDIATELY commit to git
cd ~/bed-presence-sensor
git add esphome/custom_components/bed_presence_engine/bed_presence.h
git commit -m "Quick fix: adjust threshold defaults"
git push origin main

# 5. Pull in Codespace later
# (In Codespace terminal)
cd /workspaces/bed-presence-sensor
git pull origin main
```

**Key**: Always commit and push immediately after testing succeeds.

### Option B: Copy File from Codespace to Ubuntu-node (NOT RECOMMENDED)

```bash
# In Codespace
# Copy updated file to ubuntu-node via stdin
ssh ubuntu-node "cat > ~/bed-presence-sensor/esphome/custom_components/bed_presence_engine/bed_presence.h" < /workspaces/bed-presence-sensor/esphome/custom_components/bed_presence_engine/bed_presence.h

# Flash on ubuntu-node
ssh ubuntu-node "cd ~/bed-presence-sensor/esphome && source ~/esphome-venv/bin/activate && esphome run bed-presence-detector.yaml --device /dev/ttyACM0"

# THEN: Commit in Codespace
cd /workspaces/bed-presence-sensor
git add esphome/custom_components/bed_presence_engine/bed_presence.h
git commit -m "Update baseline values"
git push origin main
```

**Why not recommended**: Bypasses git history, prone to errors, hard to track changes.

---

## Workflow 3: Baseline Calibration (Multi-step Process)

**When to use**: Following `docs/phase1-completion-steps.md` Step 2 & 3

### Step 1: Collect Baseline Data (on Ubuntu-node)

```bash
# SSH to ubuntu-node
ssh ubuntu-node
cd ~/bed-presence-sensor

# Run baseline collection script
python3 scripts/collect_baseline.py

# Script outputs:
#   Mean (μ): 6.30
#   Std Dev (σ): 2.56
# RECORD THESE VALUES
```

### Step 2: Update Code (in Codespace)

```bash
# In Codespace
cd /workspaces/bed-presence-sensor
nano esphome/custom_components/bed_presence_engine/bed_presence.h

# Update lines ~40-47 with collected values:
# float mu_move_{6.3f};     // Mean still energy (empty bed)
# float sigma_move_{2.6f};  // Std dev still energy (empty bed)
# float mu_stat_{6.3f};
# float sigma_stat_{2.6f};

# Commit immediately
git add esphome/custom_components/bed_presence_engine/bed_presence.h
git commit -m "Calibrate baseline with real sensor data

Baseline collected on $(date +%Y-%m-%d):
- Mean (μ): 6.30%
- Std Dev (σ): 2.56%
- Samples: 30 over 60 seconds
- Location: Bedroom, empty bed

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"

git push origin main
```

### Step 3: Sync and Flash (on Ubuntu-node)

```bash
# SSH to ubuntu-node
ssh ubuntu-node
cd ~/bed-presence-sensor

# Pull calibrated baseline
git pull origin main

# Verify changes
grep "mu_move_" esphome/custom_components/bed_presence_engine/bed_presence.h
# Should show: float mu_move_{6.3f};

# Flash firmware with calibrated baseline
cd esphome
source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0

# Monitor logs to verify baseline is correct
esphome logs bed-presence-detector.yaml --device /dev/ttyACM0
# Look for: "Baseline (moving): μ=6.30, σ=2.60"
```

**✅ Verification**: Logs should show calibrated baseline, NOT placeholders (100.0, 20.0)

---

## Workflow 4: Secrets Management (Special Case)

**⚠️ TWO secrets files are gitignored and must be managed manually**

### Problem

This project uses **two different secrets files** that are NOT in git:
1. `esphome/secrets.yaml` - WiFi credentials for firmware
2. `.env.local` - Home Assistant API access for scripts

Both files can get out of sync between Codespace and ubuntu-node.

### Solution: Ubuntu-node is Source of Truth for BOTH Files

**File 1: WiFi Credentials** (`esphome/secrets.yaml`)

```bash
# ALWAYS copy secrets FROM ubuntu-node TO Codespace (not the reverse)

# In Codespace: Get WiFi secrets from ubuntu-node
ssh ubuntu-node "cat ~/bed-presence-sensor/esphome/secrets.yaml" > /workspaces/bed-presence-sensor/esphome/secrets.yaml

# Verify
cat /workspaces/bed-presence-sensor/esphome/secrets.yaml
# Should show: wifi_ssid: "TP-Link_BECC" (actual SSID)
```

**File 2: Home Assistant API Access** (`.env.local`)

```bash
# In Codespace: Get HA API credentials from ubuntu-node
ssh ubuntu-node "cat ~/bed-presence-sensor/.env.local" > /workspaces/bed-presence-sensor/.env.local

# Verify
cat /workspaces/bed-presence-sensor/.env.local
# Should show: HA_URL=http://192.168.0.148:8123
#              HA_TOKEN=your-long-lived-access-token-here
```

**Why**: Ubuntu-node has the real credentials that work with the physical network and Home Assistant instance. Codespace may have placeholders or stale values.

**When to sync secrets**:
- **`secrets.yaml`**: Before compiling firmware in Codespace (to test compilation with real WiFi)
- **`.env.local`**: Before running scripts like `collect_baseline.py` in Codespace (rare - usually run on ubuntu-node)
- After credentials change (WiFi password, HA token regenerated)
- When setting up a new Codespace

**Never**:
- Commit `secrets.yaml` or `.env.local` to git
- Copy placeholder secrets from Codespace to ubuntu-node
- Reverse the direction (ubuntu-node is ALWAYS source of truth)

---

## Pre-flight Checklist (Before Flashing Firmware)

Use this checklist to avoid common mistakes:

### ✅ On Ubuntu-node:

```bash
ssh ubuntu-node
cd ~/bed-presence-sensor

# 1. Verify git is up to date
git status  # Should show: "Your branch is up to date with 'origin/main'"
git log -1  # Check latest commit matches what you expect

# 2. Verify secrets file exists and has real credentials
cat esphome/secrets.yaml | grep wifi_ssid
# Should show: wifi_ssid: "TP-Link_BECC" (or your actual SSID)
# NOT: wifi_ssid: "YOUR_WIFI_SSID_HERE"

# 3. Verify baseline calibration values (if updated)
grep "mu_move_" esphome/custom_components/bed_presence_engine/bed_presence.h
# Should show your calibrated values (e.g., 6.3f)
# NOT placeholders (100.0f)

# 4. Check device is connected
ls -la /dev/ttyACM0
# Should show: crw-rw---- 1 root dialout ... /dev/ttyACM0

# 5. Flash
cd esphome
source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

### ✅ After Flashing:

```bash
# Monitor logs for verification
esphome logs bed-presence-detector.yaml --device /dev/ttyACM0

# Look for these success indicators:
# ✅ "WiFi: Connected to 'TP-Link_BECC'" (not "YOUR_WIFI_SSID_HERE")
# ✅ "Baseline (moving): μ=6.30, σ=2.60" (your actual calibration, not 100.0, 20.0)
# ✅ "Home Assistant ... connected"
# ✅ "LD2410 Still Energy: Sending state X.XX %"
```

---

## Common Sync Issues and Fixes

### Issue 1: "I flashed firmware but it has old baseline values"

**Root cause**: Ubuntu-node has stale code, didn't pull from git

**Fix**:
```bash
ssh ubuntu-node
cd ~/bed-presence-sensor
git status  # Check if behind
git pull origin main  # Sync with GitHub
grep "mu_move_" esphome/custom_components/bed_presence_engine/bed_presence.h  # Verify
cd esphome && source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0  # Reflash
```

### Issue 2: "Device won't connect to WiFi after flashing"

**Root cause**: Firmware compiled with placeholder WiFi credentials from Codespace

**Fix**:
```bash
ssh ubuntu-node
cd ~/bed-presence-sensor/esphome

# Verify secrets has real credentials
cat secrets.yaml | grep wifi_ssid
# If shows placeholders, copy from backup:
cat secrets.yaml.example  # NO! This has placeholders too

# Get actual credentials (stored on ubuntu-node)
cat ~/esphome-venv/secrets-backup.yaml  # If you made a backup
# Or manually edit:
nano secrets.yaml
# Set: wifi_ssid: "TP-Link_BECC"
#      wifi_password: "actual_password"

# Reflash
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

**Prevention**: Always use ubuntu-node as source of truth for `secrets.yaml`

### Issue 3: "Codespace and ubuntu-node code differ"

**Root cause**: Made changes in one location without syncing

**Fix**:
```bash
# Determine which has the "correct" code
# Usually: Codespace has newer changes if you were editing there

# On ubuntu-node: Check git status
ssh ubuntu-node
cd ~/bed-presence-sensor
git status
# If shows uncommitted changes:
git diff  # Review changes
# Option A: Discard local changes (if Codespace is correct)
git reset --hard origin/main
git pull origin main

# Option B: Commit local changes (if ubuntu-node is correct)
git add -A
git commit -m "Fix: sync ubuntu-node changes"
git push origin main
# Then pull in Codespace
```

---

## Workflow Best Practices Summary

### ✅ DO:
1. **Edit code in Codespace** and commit to git
2. **Pull latest from git** on ubuntu-node before flashing
3. **Use git log** to verify correct commit is checked out
4. **Keep secrets.yaml on ubuntu-node** as source of truth
5. **Run pre-flight checklist** before flashing firmware
6. **Monitor logs after flashing** to verify correct baseline and WiFi

### ❌ DON'T:
1. **Edit code directly on ubuntu-node** without immediately committing
2. **Flash firmware** without pulling latest from git
3. **Copy secrets.yaml from Codespace to ubuntu-node** (wrong direction!)
4. **Assume ubuntu-node is up to date** - always `git pull` first
5. **Skip verification** - always check logs after flashing

### 🔄 Mental Model:

```
Codespace (Dev) → Git (Source of Truth) → Ubuntu-node (Flash) → M5Stack
     ▲                                            │
     └────────── git pull (sync back) ────────────┘
```

---

## Direct Workflow (Legacy - Single Location)

**When to use**: Direct SSH development without Codespace

### 1. Connect to Node via SSH

```bash
ssh ubuntu-node
```

### 2. Navigate to Project

```bash
cd ~/bed-presence-sensor/esphome
source ~/esphome-venv/bin/activate
```

### 3. Make Code Changes

Edit files in `custom_components/bed_presence_engine/` or YAML configurations.

### 4. Test and Flash

```bash
# Run unit tests (fast, no hardware needed)
cd ~/bed-presence-sensor/esphome
platformio test -e native

# Compile and flash to device
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

### 5. Monitor Device

```bash
# View live logs
esphome logs bed-presence-detector.yaml --device /dev/ttyACM0

# Or view in Home Assistant
# Go to Settings → Devices → Bed Presence Detector → Logs
```

### 6. Test in Home Assistant

- Check sensor values: Developer Tools → States
- Test threshold tuning: Adjust `k_on` and `k_off` numbers
- Monitor presence state: `binary_sensor.bed_occupied`

### 7. Commit Changes

```bash
cd ~/bed-presence-sensor
git add -A
git commit -m "Describe your changes"
git push origin main
```

## E2E Testing Setup

### Prerequisites

```bash
cd ~/bed-presence-sensor/tests/e2e
pip install -r requirements.txt
```

### Environment Variables

```bash
export HA_URL="ws://YOUR_NODE_IP:8123/api/websocket"  # Local network
# or
export HA_URL="ws://<cf-ha-hostname>:8123/api/websocket"  # Via Cloudflare tunnel

export HA_TOKEN="your-long-lived-access-token-here"
```

**Note**: Get the access token from Home Assistant: Profile → Security → Long-Lived Access Tokens

### Run Tests

```bash
cd ~/bed-presence-sensor/tests/e2e
pytest -v
```

## Baseline Calibration Workflow

### 1. Collect Baseline Data

With an empty bed, monitor `sensor.ld2410_still_energy` for 30-60 seconds:

```bash
# In Home Assistant Developer Tools → States
# Record values of sensor.ld2410_still_energy
```

Or via API:
```bash
# Get sensor value
curl -H "Authorization: Bearer $HA_TOKEN" \
     -H "Content-Type: application/json" \
     http://YOUR_NODE_IP:8123/api/states/sensor.ld2410_still_energy
```

### 2. Calculate Statistics

Calculate mean (μ) and standard deviation (σ) from recorded values.

### 3. Update Firmware

Edit `esphome/custom_components/bed_presence_engine/bed_presence.h`:

```cpp
float mu_move_{100.0f};   // Replace with calculated mean
float sigma_move_{20.0f}; // Replace with calculated std dev
```

### 4. Reflash

```bash
cd ~/bed-presence-sensor/esphome
source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

## Troubleshooting

### Device Not Detected

```bash
# Check USB device
ls -la /dev/ttyACM0
lsusb

# Check dmesg for USB events
sudo dmesg | grep -i usb | tail -20
```

### WiFi Connection Issues

```bash
# View device logs
esphome logs bed-presence-detector.yaml --device /dev/ttyACM0

# Check WiFi credentials
cat ~/bed-presence-sensor/esphome/secrets.yaml
```

### Home Assistant Connection Issues

```bash
# Check if device is reachable (get IP from HA or router)
ping YOUR_DEVICE_IP

# Check if API port is open
nc -zv YOUR_DEVICE_IP 6053

# View ESPHome logs in HA
# Settings → Devices & Services → ESPHome → Bed Presence Detector → Logs
```

### Cloudflare Tunnel Issues

```bash
# Check tunnel status
sudo systemctl status cloudflared

# Restart tunnel
sudo systemctl restart cloudflared

# View tunnel logs
sudo journalctl -u cloudflared -f
```

## Security Considerations

### Temporary Access

The `claude-temp` user is intended for temporary development only:
- Full sudo access (NOPASSWD: ALL)
- Should be removed after development session
- SSH key should be revoked

### Cleanup After Session

```bash
# Remove temporary user
sudo deluser claude-temp
sudo rm -rf /home/claude-temp

# Remove SSH key from authorized_keys
# Edit ~/.ssh/authorized_keys and remove the temporary session key

# Optional: Disable Cloudflare SSH tunnel access
# Edit /etc/cloudflared/config.yml and remove SSH ingress rule
# Then: sudo systemctl restart cloudflared
```

### API Token Security

- Long-lived access tokens should be treated as passwords
- Revoke tokens after development: Profile → Security → Long-Lived Access Tokens → Revoke
- Don't commit tokens to git

## Backup and Recovery

### Important Files to Backup

1. **Home Assistant Configuration**: `/opt/homeassistant/config/`
2. **ESPHome Secrets**: `~/bed-presence-sensor/esphome/secrets.yaml`
3. **Cloudflare Tunnel Config**: `/etc/cloudflared/config.yml`
4. **Custom Component Code**: `~/bed-presence-sensor/esphome/custom_components/`

### Recovery Steps

If the M5Stack needs to be reflashed from scratch:

1. SSH into the node
2. Navigate to project: `cd ~/bed-presence-sensor/esphome`
3. Activate venv: `source ~/esphome-venv/bin/activate`
4. Flash device: `esphome run bed-presence-detector.yaml --device /dev/ttyACM0`
5. Re-add to Home Assistant using encryption key from secrets.yaml

## Quick Reference Commands

```bash
# Connect via SSH
ssh ubuntu-node

# Activate ESPHome environment
cd ~/bed-presence-sensor/esphome
source ~/esphome-venv/bin/activate

# Flash firmware
esphome run bed-presence-detector.yaml --device /dev/ttyACM0

# View device logs
esphome logs bed-presence-detector.yaml --device /dev/ttyACM0

# Check device connection
ls -la /dev/ttyACM*

# Run unit tests
cd ~/bed-presence-sensor/esphome && platformio test -e native

# Run e2e tests
cd ~/bed-presence-sensor/tests/e2e && pytest -v

# Check Home Assistant container
docker ps | grep homeassistant

# View Cloudflare tunnel status
sudo systemctl status cloudflared
```

---

**Last Updated**: 2025-11-05
**M5Stack Firmware Version**: Phase 1 (z-score based detection)
**ESPHome Version**: 2025.10.4

**Note**: This document is sanitized for public repositories. Actual credentials, IP addresses, and domain names are stored locally on the node and in development environments.
