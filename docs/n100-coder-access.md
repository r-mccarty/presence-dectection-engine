# N100 Node & Coder Access Guide

This guide covers accessing the N100 development node and Coder workspaces, including
setup for AI agents.

## Infrastructure Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         Internet                                │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                   Cloudflare Tunnel
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
 ssh.hardwareos.com  coder.hardwareos.com  ha.hardwareos.com
        │                   │                   │
        └───────────────────┼───────────────────┘
                            │
                            ▼
                ┌───────────────────────┐
                │  N100 Mini PC         │
                │  (home-assistant-node)│
                │  192.168.0.148 (LAN)  │
                ├───────────────────────┤
                │  • Docker             │
                │  • Coder (port 7080)  │
                │  • Home Assistant     │
                │  • Cloudflared        │
                └───────────┬───────────┘
                            │
                            ▼
                ┌───────────────────────┐
                │  Luckfox Pico Max     │
                │  172.32.0.93 (RNDIS)  │
                └───────────────────────┘
```

## Quick Reference

| Resource | URL/Address | Auth |
|----------|-------------|------|
| **Coder UI** | https://coder.hardwareos.com | Username/password |
| **SSH to N100** | `ssh.hardwareos.com` | SSH key via cloudflared |
| **Home Assistant** | https://ha.hardwareos.com | HA login |
| **N100 LAN IP** | `192.168.0.148` | Only from local network |
| **Luckfox** | `172.32.0.93` | `root`/`luckfox` |

---

## Coder Access

### Web UI

Access the Coder dashboard at:

```
https://coder.hardwareos.com
```

### API Authentication

For programmatic access (scripts, AI agents, CI/CD):

1. **Create an API token**:
   - Login to https://coder.hardwareos.com
   - Click avatar → **Settings** → **Tokens**
   - Create token with desired expiration

2. **Use the token**:
   ```bash
   export CODER_URL="https://coder.hardwareos.com"
   export CODER_SESSION_TOKEN="your-token-here"

   # List workspaces
   coder list

   # Create a workspace
   coder create my-workspace --template opticworks-dev

   # Start/stop
   coder start my-workspace
   coder stop my-workspace

   # Execute command in workspace
   coder ssh my-workspace -- ls -la
   ```

### Coder CLI Installation

```bash
# macOS/Linux
curl -fsSL https://coder.com/install.sh | sh

# Or via npm
npm install -g @coder/cli
```

---

## SSH Access to N100 Node

### Prerequisites

1. **cloudflared** installed on your machine:
   ```bash
   # macOS
   brew install cloudflared

   # Ubuntu/Debian
   curl -fsSL https://pkg.cloudflare.com/cloudflare-main.gpg | sudo tee /usr/share/keyrings/cloudflare.gpg
   echo "deb [signed-by=/usr/share/keyrings/cloudflare.gpg] https://pkg.cloudflare.com/cloudflared $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/cloudflared.list
   sudo apt update && sudo apt install cloudflared
   ```

2. **SSH key** authorized on the node

### SSH Config

Add to `~/.ssh/config`:

```ssh-config
Host n100
  HostName ssh.hardwareos.com
  User claude-temp
  IdentityFile ~/.ssh/your-key
  ProxyCommand cloudflared access ssh --hostname ssh.hardwareos.com
  StrictHostKeyChecking no
  UserKnownHostsFile /dev/null
```

Then connect:

```bash
ssh n100
```

### Direct SSH (from LAN only)

If on the same local network:

```bash
ssh ryan@192.168.0.148
```

---

## AI Agent Access

### What an AI Agent Needs

For an AI agent (Claude, GPT, etc.) to interact with Coder workspaces:

| Requirement | How to Provide |
|-------------|----------------|
| **Coder API Token** | Store in Infisical or pass as env var |
| **Coder URL** | `https://coder.hardwareos.com` |
| **Workspace name** | Create via API or pre-provision |
| **SSH access** | Via `coder ssh <workspace>` |

### Agent Workflow Example

```bash
# 1. Set credentials (from Infisical or env)
export CODER_URL="https://coder.hardwareos.com"
export CODER_SESSION_TOKEN="$CODER_API_TOKEN"

# 2. Create or start a workspace
coder create agent-workspace --template opticworks-dev --yes 2>/dev/null || \
coder start agent-workspace

# 3. Wait for workspace to be ready
while [ "$(coder show agent-workspace -o json | jq -r '.latest_build.status')" != "running" ]; do
  sleep 5
done

# 4. Execute commands in the workspace
coder ssh agent-workspace -- bash -c '
  cd ~/workspace
  git clone https://github.com/example/repo.git
  cd repo
  npm install
  npm test
'

# 5. Stop workspace when done (optional, saves resources)
coder stop agent-workspace
```

### Infisical Integration

Workspaces can auto-inject secrets from Infisical on startup:

```
Infisical Token    → INFISICAL_TOKEN (service token)
Infisical Project  → Project ID from dashboard
Infisical Env      → dev / staging / prod
```

Secrets are exported to `/home/coder/.env.secrets` and sourced in `.bashrc`.

### Available AI CLI Tools in Workspaces

The `opticworks-dev` template includes:

| Tool | Command | Auth Env Var |
|------|---------|--------------|
| Claude Code | `claude` | `ANTHROPIC_API_KEY` |
| Gemini CLI | `gemini` | `GOOGLE_API_KEY` |
| OpenAI Codex | `codex` | `OPENAI_API_KEY` |
| OpenCode | `opencode` | Various (see docs) |

---

## Coder API Reference

### List Workspaces

```bash
curl -s -H "Coder-Session-Token: $CODER_SESSION_TOKEN" \
  "$CODER_URL/api/v2/workspaces" | jq '.workspaces[].name'
```

### Create Workspace

```bash
curl -s -X POST -H "Coder-Session-Token: $CODER_SESSION_TOKEN" \
  -H "Content-Type: application/json" \
  "$CODER_URL/api/v2/organizations/default/members/me/workspaces" \
  -d '{
    "name": "my-workspace",
    "template_id": "<template-uuid>",
    "rich_parameter_values": [
      {"name": "workspace_type", "value": "agent"},
      {"name": "enable_usb", "value": "false"}
    ]
  }'
```

### Get Workspace Status

```bash
curl -s -H "Coder-Session-Token: $CODER_SESSION_TOKEN" \
  "$CODER_URL/api/v2/workspaces/<workspace-id>" | jq '.latest_build.status'
```

### Start/Stop Workspace

```bash
# Start
curl -s -X POST -H "Coder-Session-Token: $CODER_SESSION_TOKEN" \
  "$CODER_URL/api/v2/workspaces/<workspace-id>/builds" \
  -d '{"transition": "start"}'

# Stop
curl -s -X POST -H "Coder-Session-Token: $CODER_SESSION_TOKEN" \
  "$CODER_URL/api/v2/workspaces/<workspace-id>/builds" \
  -d '{"transition": "stop"}'
```

---

## Hardware Access from Workspaces

### USB Devices

Workspaces with `enable_usb=true` have:
- Privileged Docker mode
- `/dev` mounted from host
- Host network mode

```bash
# List USB devices
lsusb

# Access Luckfox via RNDIS
ping 172.32.0.93
ssh root@172.32.0.93
```

### Serial Ports

```bash
# List serial ports
ls /dev/ttyUSB* /dev/ttyACM*

# Connect to serial console
picocom -b 115200 /dev/ttyUSB0
```

---

## Troubleshooting

### Cannot Connect to Coder

```bash
# Check if Coder is responding
curl -s https://coder.hardwareos.com/api/v2 | jq .

# Verify token is valid
curl -s -H "Coder-Session-Token: $CODER_SESSION_TOKEN" \
  "https://coder.hardwareos.com/api/v2/users/me" | jq .username
```

### SSH via Cloudflare Fails

```bash
# Test cloudflared connectivity
cloudflared access ssh --hostname ssh.hardwareos.com --destination localhost:22

# Check if tunnel is up (from N100)
sudo systemctl status cloudflared
```

### Workspace Won't Start

```bash
# Check workspace logs
coder show my-workspace --log

# Check Docker on N100
ssh n100 "docker ps -a | grep coder"
ssh n100 "docker logs coder-<owner>-<workspace>"
```

---

## Infisical Secrets Management

### Overview

Infisical is used to manage API keys and secrets across all workspaces. Secrets are
automatically injected on workspace startup.

### Infisical Access

| Resource | URL |
|----------|-----|
| **Dashboard** | https://app.infisical.com |
| **Docs** | https://infisical.com/docs |

### Creating a Service Token

1. Login to https://app.infisical.com
2. Select your project (e.g., `opticworks`)
3. Go to **Project Settings** → **Service Tokens**
4. Create token with:
   - **Name**: Descriptive name (e.g., `coder-dev-token`)
   - **Scopes**: Select environments (`dev`, `staging`, `prod`)
   - **Permissions**: Read (or Read/Write if agent needs to update secrets)
5. Copy the token (starts with `st.`)

### Required Secrets

Store these in Infisical for AI agent workspaces:

```
# AI Provider Keys
ANTHROPIC_API_KEY=sk-ant-api03-...
OPENAI_API_KEY=sk-...
GOOGLE_API_KEY=AIza...

# Coder Access (for agent self-management)
CODER_SESSION_TOKEN=...
CODER_URL=https://coder.hardwareos.com

# Optional: GitHub for cloning private repos
GITHUB_TOKEN=ghp_...
```

### Using Infisical CLI

```bash
# Install
curl -1sLf "https://dl.cloudsmith.io/public/infisical/infisical-cli/setup.deb.sh" | sudo bash
sudo apt install infisical

# Login (interactive)
infisical login

# Export secrets to environment
eval $(infisical export --env=dev --format=dotenv-export)

# Or export to file
infisical export --env=dev --format=dotenv > .env

# Run command with secrets injected
infisical run --env=dev -- npm start
```

### Infisical in Coder Workspaces

When creating a workspace, provide:

| Parameter | Description |
|-----------|-------------|
| `infisical_token` | Service token (st.xxx) |
| `infisical_project` | Project ID from Infisical dashboard |
| `infisical_env` | Environment: `dev`, `staging`, or `prod` |

Secrets are automatically:
1. Exported to `/home/coder/.env.secrets`
2. Sourced in `.bashrc` for all terminal sessions
3. Available to all AI CLI tools

### Infisical API (for programmatic access)

```bash
# Get secrets via API
curl -s -H "Authorization: Bearer $INFISICAL_TOKEN" \
  "https://app.infisical.com/api/v3/secrets/raw?workspaceId=$PROJECT_ID&environment=dev" \
  | jq '.secrets[] | {key: .secretKey, value: .secretValue}'
```

### Project Structure Recommendation

```
Infisical Project: opticworks
├── dev/
│   ├── ANTHROPIC_API_KEY
│   ├── OPENAI_API_KEY
│   ├── GOOGLE_API_KEY
│   └── CODER_SESSION_TOKEN
├── staging/
│   └── (same keys, different values)
└── prod/
    └── (production keys)
```

---

## Security Notes

- **API tokens**: Store securely in Infisical or a secrets manager
- **SSH keys**: Use Ed25519, rotate regularly
- **Cloudflare Tunnel**: All traffic is encrypted end-to-end
- **Workspaces**: Isolated Docker containers with user namespacing
- **Privileged mode**: Only enable USB access when needed
