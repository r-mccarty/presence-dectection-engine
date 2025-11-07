# AGENTS.md – Documentation

Guidelines for agents editing Markdown under `docs/`. Treat `CLAUDE.md` and `README.md` as companions; this directory provides the deeper engineering + operator material referenced there.

## Document Topology
```
docs/
├── presence-engine-spec.md          # Engineering roadmap + detailed requirements (Phases 1-3)
├── phase1-hardware-setup.md         # Wiring, baseline collection, calibration walkthrough
├── phase1-completion-steps.md       # Verification checklist for Phase 1 deliverables
├── phase2-completion-steps.md       # Verification checklist for Phase 2 state machine
├── quickstart.md                    # User onboarding guide
├── calibration.md                   # Manual + planned automated calibration workflows
├── faq.md                           # Frequently asked questions
├── troubleshooting.md               # Problem / cause / fix reference
├── gitops-deployment-guide.md       # CI/CD + OTA deployment workflow
├── self-hosted-runner-setup.md      # GitHub Actions runner instructions
├── ubuntu-node-setup.md             # Preparing a dedicated Ubuntu HA node
├── RFD-001-still-vs-moving-energy.md# Design decision on still vs moving energy sources
└── assets/                          # Images, diagrams (keep filenames descriptive)
```

## Authoring Principles
1. **Accuracy first** – reflect actual firmware defaults (Phase 2: `k_on 9.0`, `k_off 4.0`, `on/off debounce 3s/5s`, `absolute_clear 30s`).
2. **Phase markers** – clearly mark whether content applies to Phase 1, Phase 2 (current), or Phase 3 (planned). Use callouts when describing planned work.
3. **Audience clarity** – engineering specs can assume C++/ESPHome familiarity; quickstart/FAQ must stay approachable for Home Assistant users.
4. **Update everything** – when behavior changes, touch all affected docs (spec, completion steps, quickstart, troubleshooting, README, dashboards) so nothing drifts.

## Style Guide
- Markdown, 2-space indentation inside lists and code fences, ≤120 character lines.
- Use fenced code blocks with language hints (` ```yaml`, ` ```cpp`, ` ```bash`).
- Prefer active voice and present tense.
- Call out warnings with blockquotes: `> **Warning**: ...`
- Link to related sections using relative paths (`[Phase 2 checklist](phase2-completion-steps.md)`).
- Embed images from `assets/` with descriptive alt text.

## Review Checklist Before Commit
- [ ] Facts match firmware + HA configuration (entity IDs, defaults, state names).
- [ ] Screenshots/diagrams updated if UI changed.
- [ ] Internal/external links work (use `markdown-link-check` if unsure).
- [ ] Spelling/grammar checked (`cspell` or editor tools).
- [ ] Phase references accurate.
- [ ] Tables + lists render correctly in GitHub preview.

## Helpful Commands
```bash
# Lint Markdown (install markdownlint-cli first)
markdownlint docs/**/*.md

# Spell check (install cspell first)
cspell "docs/**/*.md"

# Search for outdated values
rg "k_on" docs/
rg "Phase 1" docs/
```

Need repo-wide context? See `../AGENTS.md`.
