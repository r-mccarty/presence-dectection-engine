# GitOps Deployment Guide

This guide explains how the GitOps-based preview deployment workflow operates and how to use it for automated testing.

## Overview

The preview deployment workflow provides **automated deployment and testing** for every pull request:

1. **Compile firmware** on GitHub-hosted runners (fast, no hardware needed)
2. **Deploy to test hardware** via self-hosted runner with network access
3. **Run E2E tests** against live Home Assistant + ESPHome device
4. **Report results** directly in the PR with status checks

This enables **hardware-in-the-loop testing** without manual intervention.

## Workflow Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  Pull Request Created/Updated                                    │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────────┐
│  Job 1: validate-firmware (GitHub-hosted runner)                │
│  - Compile ESPHome firmware                                      │
│  - Upload firmware artifact                                      │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────────┐
│  Job 2: unit-tests (GitHub-hosted runner)                       │
│  - Run C++ unit tests via PlatformIO                            │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────────┐
│  Job 3: preview-deployment (Self-hosted runner)                 │
│  - Deploy firmware to M5Stack via OTA                           │
│  - Deploy HA configuration via API                              │
│  - Run E2E tests against live hardware                          │
│  - Upload test results                                           │
│  - Comment on PR with results                                   │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────────┐
│  PR Review with automated test results                          │
└─────────────────────────────────────────────────────────────────┘
```

## Workflow Files

### Main Workflow: `.github/workflows/preview_deployment.yml`

**Triggers**:
- Pull request opened, synchronized (new commits), or reopened
- Manual workflow dispatch (for testing)

**Jobs**:

#### 1. `validate-firmware`
- Runs on GitHub-hosted `ubuntu-latest`
- Compiles ESPHome firmware
- Uploads firmware as artifact for debugging
- Fast (2-3 minutes)

#### 2. `unit-tests`
- Runs on GitHub-hosted `ubuntu-latest`
- Executes C++ unit tests via PlatformIO
- No hardware required
- Fast (1-2 minutes)

#### 3. `preview-deployment`
- Runs on **self-hosted runner** (requires setup)
- Deploys to physical hardware
- Runs E2E integration tests
- Duration: 5-10 minutes (includes device reboot)

**Environment**: Creates dynamic environment `preview-<PR_NUMBER>` for tracking

#### 4. `cleanup-preview` (on PR close)
- Runs on **self-hosted runner**
- Rolls back hardware to `main` branch state
- Ensures test environment is clean for next PR

## Deployment Scripts

### 1. `scripts/deploy_esphome.sh`

Handles ESPHome firmware compilation and deployment.

**Usage**:
```bash
./scripts/deploy_esphome.sh \
  --device "192.168.1.100" \
  --password "your-ota-password" \
  --config "esphome/bed-presence-detector.yaml"
```

**Deployment Methods**:

- `--method ota` (default): Deploy via network OTA update
  - Requires device to be online and reachable
  - Fastest for iterative development
  - Automatically verifies deployment

- `--method usb`: Deploy via USB serial connection
  - Requires physical USB connection to runner
  - Useful for initial flashing or recovery
  - Slower than OTA

- `--method upload-only`: Compile only, no deployment
  - For CI validation without hardware
  - Firmware saved as artifact

**Features**:
- Pre-deployment device reachability check
- Automatic compilation
- Post-deployment verification
- Colored output for CI logs
- Configurable timeout for device boot

**Exit Codes**:
- `0`: Success
- `1`: Compilation failed, device unreachable, or verification failed

### 2. `scripts/deploy_homeassistant.sh`

Deploys Home Assistant configuration (blueprints, dashboards, helpers).

**Usage**:
```bash
./scripts/deploy_homeassistant.sh \
  --ha-host "192.168.1.50" \
  --ha-token "your-long-lived-token" \
  --config-path "homeassistant/"
```

**Deployment Methods**:

- `--method api` (default): Deploy via Home Assistant REST API
  - Verifies connectivity
  - Reloads automations and scripts
  - Validates entity existence

- `--method filesystem`: Direct file copy to HA config directory
  - Requires filesystem access (SSH, NFS, or local)
  - More reliable for large configurations
  - Requires HA restart to take effect

**Features**:
- API connectivity check
- Entity verification (checks if ESPHome device entities exist)
- Configuration reload (automations, scripts)
- Blueprint deployment
- Dry-run mode for testing

**Exit Codes**:
- `0`: Success
- `1`: HA unreachable or API error

### 3. `scripts/run_e2e_tests.sh`

Executes E2E integration tests against live hardware.

**Usage**:
```bash
./scripts/run_e2e_tests.sh \
  --ha-url "ws://192.168.1.50:8123/api/websocket" \
  --ha-token "your-token" \
  --device-name "bed_presence_detector"
```

**Features**:
- Pre-flight checks (HA connectivity, device integration)
- Automatic dependency installation
- JUnit XML report generation for GitHub
- Verbose mode for debugging
- Environment variable support

**Test Phases**:
1. Verify pytest installation
2. Install test dependencies (`tests/e2e/requirements.txt`)
3. Check HA API connectivity
4. Verify ESPHome device entities exist
5. Run pytest with JUnit XML reporting
6. Generate summary report

**Exit Codes**:
- `0`: All tests passed
- `1+`: Tests failed (exit code indicates failure count)

## Repository Secrets

Configure these in **Settings → Secrets and variables → Actions → Repository secrets**:

### ESPHome Device Secrets

| Secret | Description | How to Get |
|--------|-------------|------------|
| `PREVIEW_WIFI_SSID` | WiFi network name | Your network SSID |
| `PREVIEW_WIFI_PASSWORD` | WiFi password | Your network password |
| `PREVIEW_API_KEY` | ESPHome API encryption key | Generate: `python3 -c "import secrets; print(secrets.token_urlsafe(32))"` |
| `PREVIEW_OTA_PASSWORD` | ESPHome OTA password | Generate: `python3 -c "import secrets; print(secrets.token_urlsafe(16))"` |
| `ESPHOME_DEVICE_HOST` | M5Stack IP or hostname | Check your router or use mDNS: `bed-presence-detector.local` |

### Home Assistant Secrets

| Secret | Description | How to Get |
|--------|-------------|------------|
| `HA_HOST` | Home Assistant IP or hostname | Your HA IP address or `homeassistant.local` |
| `HA_TOKEN` | Long-lived access token | HA Profile → Long-Lived Access Tokens → Create Token |

## Using the Preview Deployment Workflow

### Creating a Pull Request

1. **Create a feature branch**:
   ```bash
   git checkout -b feature/my-improvement
   ```

2. **Make your changes** to firmware, configuration, or tests

3. **Commit and push**:
   ```bash
   git add .
   git commit -m "feat: add new feature"
   git push origin feature/my-improvement
   ```

4. **Create a pull request** on GitHub

5. **Automatic deployment starts**:
   - GitHub Actions will automatically trigger
   - Check the "Actions" tab to watch progress
   - PR checks will update with status

### Monitoring Deployment

**In the GitHub Actions UI**:
- Navigate to **Actions** → Select the workflow run
- View real-time logs for each job
- Download artifacts (firmware binaries, test reports)

**Job statuses**:
- ✅ Green checkmark: Success
- ❌ Red X: Failed (click to see logs)
- 🟡 Yellow dot: In progress
- ⚪ Gray circle: Queued or skipped

**PR Comment**:
After deployment completes, a bot comment will appear with:
- Deployment status summary
- Links to preview environment
- Test results summary
- Artifacts (firmware, test reports)

### Interpreting Results

#### All Green ✅

```
Deployment Status: Success

Component Status:
- Firmware Compilation: ✅ Success
- Unit Tests: ✅ Passed
- ESPHome Deploy: ✅ Success
- Home Assistant Deploy: ✅ Success
- E2E Tests: ✅ Passed
```

**Meaning**: Your changes compile, pass all tests, and work on real hardware. Ready to merge!

#### Partial Failures ⚠️

**Firmware compilation failed**:
- Check ESPHome YAML syntax
- Review compiler errors in job logs
- Test locally: `cd esphome && esphome compile bed-presence-detector.yaml`

**Unit tests failed**:
- Check C++ test failures in job logs
- Run locally: `cd esphome && platformio test -e native`
- Fix the failing test cases

**ESPHome deploy failed**:
- Device may be offline or unreachable
- Check OTA password matches device config
- Verify device is on same network as runner
- Check runner logs for network errors

**HA deploy failed**:
- Check HA token is valid
- Verify HA is reachable from runner
- Check for API errors in logs

**E2E tests failed**:
- Review test logs (uploaded as artifact)
- Check if device entities exist in HA
- Run tests manually for detailed output
- Verify entity names match Phase 1 expectations

### Re-running Failed Jobs

If a job fails due to transient issues (network timeout, etc.):

1. Go to **Actions** → select the failed run
2. Click **Re-run jobs** → **Re-run failed jobs**
3. GitHub will retry only the failed jobs

### Viewing Test Artifacts

After workflow completion:

1. Go to workflow run page
2. Scroll to **Artifacts** section
3. Download:
   - `firmware-<PR_NUMBER>`: Compiled firmware binaries
   - `e2e-test-results-<PR_NUMBER>`: JUnit XML and logs

**View JUnit XML** in GitHub:
- GitHub automatically parses JUnit XML
- Check "Checks" tab on PR for test details
- Failed tests show assertion messages

## Manual Deployment (Without PR)

For testing the deployment process locally or debugging:

### Deploy ESPHome Manually

```bash
# From repository root
cd esphome

# Create secrets.yaml (if not exists)
cp secrets.yaml.example secrets.yaml
# Edit secrets.yaml with your credentials

# Deploy via OTA
../scripts/deploy_esphome.sh \
  --device "192.168.1.100" \
  --password "your-ota-password" \
  --config "bed-presence-detector.yaml"
```

### Deploy Home Assistant Manually

```bash
# From repository root
./scripts/deploy_homeassistant.sh \
  --ha-host "192.168.1.50" \
  --ha-token "your-token" \
  --config-path "homeassistant/" \
  --dry-run  # Remove --dry-run when ready
```

### Run E2E Tests Manually

```bash
# From repository root
./scripts/run_e2e_tests.sh \
  --ha-url "ws://192.168.1.50:8123/api/websocket" \
  --ha-token "your-token" \
  --verbose  # For detailed output
```

## Advanced Usage

### Triggering Workflow Manually

For testing without creating a PR:

1. Go to **Actions** → **Preview Deployment**
2. Click **Run workflow**
3. Select branch and optionally specify PR number
4. Click **Run workflow**

### Skipping Deployment for Docs-Only Changes

Add `[skip ci]` or `[ci skip]` to commit message:

```bash
git commit -m "docs: update README [skip ci]"
```

This skips all CI/CD workflows.

### Testing Different Branches

The workflow automatically uses the branch from the PR. To test a different configuration:

1. Create a PR from your branch
2. Workflow deploys your branch code
3. Close PR when done (triggers cleanup)

### Parallel Testing (Future Enhancement)

To test multiple configurations simultaneously:

1. Set up multiple self-hosted runners
2. Label them differently (e.g., `runner-1`, `runner-2`)
3. Modify workflow to use matrix strategy
4. Each runner tests different hardware/config

## Cleanup and Rollback

### Automatic Cleanup on PR Close

When a PR is closed or merged, the `cleanup-preview` job:

1. Checks out the `main` branch
2. Deploys `main` branch code to hardware
3. Comments on PR with cleanup status

This ensures test hardware always returns to stable state.

### Manual Rollback

If something goes wrong:

```bash
# Checkout main branch
git checkout main

# Deploy main branch to device
cd esphome
../scripts/deploy_esphome.sh \
  --device "192.168.1.100" \
  --password "your-ota-password" \
  --config "bed-presence-detector.yaml"

# Deploy main HA config
../scripts/deploy_homeassistant.sh \
  --ha-host "192.168.1.50" \
  --ha-token "your-token" \
  --config-path "homeassistant/"
```

## Troubleshooting

### Workflow Not Triggering

**Symptoms**: PR created but no workflow run appears

**Checks**:
1. Verify workflow file exists: `.github/workflows/preview_deployment.yml`
2. Check workflow syntax: Use GitHub's workflow editor validator
3. Ensure workflow is enabled: **Actions** → **Workflows** → Enable if disabled
4. Check if `[skip ci]` is in commit message

### Self-Hosted Runner Offline

**Symptoms**: Job stuck in "Queued" state waiting for runner

**Solutions**:
1. Check runner status: **Settings** → **Actions** → **Runners**
2. SSH to runner host and check service: `sudo systemctl status actions.runner.*`
3. Restart runner: `sudo systemctl restart actions.runner.*`
4. Check runner logs: `journalctl -u actions.runner.* -f`

### Deployment Timeout

**Symptoms**: Job exceeds timeout and is cancelled

**Solutions**:
1. Increase timeout in workflow file (default: job-level timeout)
2. Check if device is responding slowly
3. Verify network is stable (no packet loss)
4. Check runner system resources (CPU, RAM, disk)

### E2E Tests Flaky

**Symptoms**: Tests pass/fail inconsistently

**Solutions**:
1. Add retry logic to tests
2. Increase wait times for device state changes
3. Check for timing-dependent assertions
4. Verify device is stable (not rebooting during tests)

### Secrets Not Working

**Symptoms**: 401 errors, authentication failures

**Solutions**:
1. Verify secret names match workflow file exactly (case-sensitive)
2. Check for whitespace in secret values
3. Re-create secrets if modified (GitHub doesn't show old values)
4. Test secrets manually with curl commands

## Security Best Practices

### Secrets Management

- ✅ **Do**: Use GitHub encrypted secrets for all credentials
- ✅ **Do**: Rotate tokens every 90 days
- ✅ **Do**: Use unique passwords for preview environment
- ❌ **Don't**: Log secrets in workflow outputs
- ❌ **Don't**: Commit secrets to repository
- ❌ **Don't**: Share secrets via insecure channels

### Network Security

- ✅ **Do**: Use firewall rules to limit runner access
- ✅ **Do**: Place devices on isolated VLAN
- ✅ **Do**: Use WPA3 for WiFi if possible
- ❌ **Don't**: Expose HA or devices to public internet
- ❌ **Don't**: Use default passwords

### Access Control

- ✅ **Do**: Require PR reviews before merge
- ✅ **Do**: Use branch protection rules
- ✅ **Do**: Limit self-hosted runner access
- ❌ **Don't**: Allow unauthenticated workflow runs
- ❌ **Don't**: Grant unnecessary permissions

## Monitoring and Metrics

### Workflow Performance

Track these metrics to identify bottlenecks:

- **Firmware compilation time**: Target < 3 minutes
- **OTA upload time**: Target < 2 minutes
- **Device boot time**: Typically 20-30 seconds
- **E2E test duration**: Target < 5 minutes
- **Total workflow time**: Target < 15 minutes

### Cost Optimization

- **GitHub-hosted runners**: Free for public repos, metered for private
- **Self-hosted runners**: Free compute, but requires maintenance
- **Artifact storage**: Free for 90 days, then charged

**Tips**:
- Use GitHub-hosted for compilation (fast, parallel)
- Use self-hosted only for hardware access
- Set artifact retention to 7 days: `retention-days: 7`

## Future Enhancements

### Potential Improvements

1. **Multi-device testing**: Test across multiple hardware configurations
2. **Performance benchmarking**: Track memory usage, response times
3. **Automatic baseline calibration**: Run calibration as part of E2E tests
4. **Integration tests**: Test with multiple sensors/actuators
5. **Snapshot testing**: Compare sensor readings to known-good baselines
6. **Notification system**: Slack/Discord alerts for failures

### Contributing

See `CONTRIBUTING.md` for guidelines on improving the GitOps workflow.

## Summary

The GitOps workflow provides:

✅ **Automated testing** on real hardware
✅ **Fast feedback** on PRs (< 15 minutes)
✅ **Consistent** environment (no "works on my machine")
✅ **Safe** deployments (automatic rollback)
✅ **Transparent** process (all results visible in PR)

This enables confident, rapid iteration on firmware and configuration changes.

## Additional Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Self-Hosted Runner Setup Guide](./self-hosted-runner-setup.md)
- [ESPHome OTA Documentation](https://esphome.io/components/ota.html)
- [Home Assistant API Documentation](https://developers.home-assistant.io/docs/api/rest/)
- [pytest Documentation](https://docs.pytest.org/)
