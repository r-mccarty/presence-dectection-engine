# Luckfox Pico Max Setup Guide

This guide covers connecting and accessing the Luckfox Pico Max (RV1106G3) development
board from the N100 node and Coder workspaces.

## Hardware Overview

| Component | Details |
|-----------|---------|
| Board | Luckfox Pico Max |
| SoC | Rockchip RV1106G3 |
| Connection | USB-C (RNDIS + ADB) |
| Host | N100 Mini PC (ubuntu-node) |

## Network Configuration

### RNDIS (USB Networking)

When connected via USB-C, the Luckfox presents itself as a USB network device using RNDIS
(Remote Network Driver Interface Specification).

| Parameter | Value |
|-----------|-------|
| **Luckfox IP** | `172.32.0.93` (static) |
| **Host IP** | `172.32.0.1/24` |
| **Interface** | `enx*` (USB ethernet, e.g., `enx5aef472011ac`) |
| **Subnet** | `172.32.0.0/24` |

### USB Device Identification

```
Bus 001 Device 003: ID 2207:0019 Fuzhou Rockchip Electronics Company rk3xxx
```

## Host Configuration (N100)

### Automatic Interface Setup

When the Luckfox is plugged in, the RNDIS interface appears but needs an IP address:

```bash
# Find the interface name
ip link show | grep enx

# Assign static IP to host side
sudo ip addr add 172.32.0.1/24 dev enxXXXXXXXXXXXX

# Verify connectivity
ping 172.32.0.93
```

### Persistent Configuration (NetworkManager)

To make the IP assignment persistent:

```bash
# Create NetworkManager connection
sudo nmcli con add type ethernet con-name luckfox ifname enx* \
  ipv4.method manual ipv4.addresses 172.32.0.1/24

# Or create a udev rule for automatic setup
sudo tee /etc/udev/rules.d/99-luckfox.rules << 'EOF'
ACTION=="add", SUBSYSTEM=="net", DRIVERS=="rndis_host", RUN+="/sbin/ip addr add 172.32.0.1/24 dev %k"
EOF
sudo udevadm control --reload-rules
```

## Accessing the Luckfox

### SSH Access

```bash
ssh root@172.32.0.93
# Default password: luckfox
```

### From Coder Workspace

Coder workspaces using the `opticworks-dev` template have:
- **Host network mode**: Direct access to `172.32.0.x` subnet
- **Privileged mode**: Full USB/device access
- **Serial tools**: `picocom`, `minicom`, `screen`

```bash
# From inside a Coder workspace terminal
ssh root@172.32.0.93

# Or use ADB if board is in USB boot mode
adb devices
adb shell
```

### Serial Console (if needed)

If the board exposes a serial port:

```bash
# Find the serial device
ls /dev/ttyUSB* /dev/ttyACM*

# Connect with picocom
picocom -b 115200 /dev/ttyUSB0
```

## Flashing Firmware

### Using rkdeveloptool (Rockchip)

```bash
# Install rkdeveloptool
sudo apt install rkdeveloptool

# Check device in maskrom/loader mode
sudo rkdeveloptool ld

# Flash firmware
sudo rkdeveloptool db MiniLoaderAll.bin
sudo rkdeveloptool wl 0 firmware.img
sudo rkdeveloptool rd
```

### Using ADB Sideload

```bash
adb reboot bootloader
adb sideload update.zip
```

## Direct Ethernet Connection

For higher bandwidth or when USB is unavailable, connect the Luckfox directly to the N100
via Ethernet:

1. Connect Ethernet cable between Luckfox and N100's `enp1s0` port
2. Configure static IP on N100:
   ```bash
   sudo ip addr add 192.168.1.1/24 dev enp1s0
   ```
3. Configure Luckfox (via SSH over RNDIS first):
   ```bash
   # On Luckfox
   ip addr add 192.168.1.100/24 dev eth0
   ```

## Troubleshooting

### Interface Not Appearing

```bash
# Check USB device detection
lsusb | grep Rockchip
dmesg | tail -20

# Reload RNDIS driver
sudo modprobe -r rndis_host
sudo modprobe rndis_host
```

### Cannot Ping Luckfox

```bash
# Verify interface has IP
ip addr show | grep 172.32

# Check if Luckfox is responding to ARP
arping -I enxXXX 172.32.0.93

# Try different default IPs (varies by firmware)
ping 172.32.0.93    # Common default
ping 192.168.123.1  # Alternative
```

### SSH Connection Refused

```bash
# Check if SSH is running on Luckfox (via serial console)
systemctl status sshd

# Or try ADB shell
adb shell
```

## Coder Integration

The `opticworks-dev` Coder template is pre-configured for Luckfox access:

| Feature | Configuration |
|---------|---------------|
| Network | `network_mode = "host"` |
| Devices | `/dev` mounted with full access |
| Privileges | `privileged = true` |
| Tools | ADB, picocom, minicom, screen |

Create a workspace at https://coder.hardwareos.com and select the `opticworks-dev` template
with USB/Hardware Access enabled.
