# Brand Guidelines: Bed Presence Sensor

**Last Updated:** 2025-11-10
**Version:** 1.0

---

## Brand Overview

### Brand Mission
To empower smart home enthusiasts with the most reliable, transparent, and privacy-respecting presence detection technology available—eliminating the frustrations of false triggers and "black box" sensors through innovative statistical intelligence.

### Brand Vision
We believe the next generation of smart home devices should be:
- **Intelligent, not reactive** - Making decisions based on statistical confidence, not raw sensor data
- **Transparent, not opaque** - Showing users exactly why a device makes each decision
- **Privacy-first, not surveillance** - Detecting presence without cameras or identifying information
- **Tunable, not locked** - Giving power users full control over every parameter

---

## Brand Positioning

### Tagline
**"Stop Detecting Motion. Start Understanding Presence."**

### Elevator Pitch
The Bed Presence Sensor isn't just another sensor—it's an intelligent presence engine for your smart home. Using a statistical algorithm and temporal filtering, it offers rock-solid, reliable bed occupancy detection that virtually eliminates the false positives and negatives that plague other solutions.

### Market Position
We position ourselves as the **technical leader** in the bed presence detection market—not the cheapest, but the most sophisticated and reliable solution for discerning smart home enthusiasts who demand precision and transparency.

**We are NOT:**
- A plug-and-play consumer product for non-technical users
- A budget alternative to pressure mats
- A complete sleep tracking system

**We ARE:**
- The gold standard for Home Assistant bed presence detection
- A sophisticated DIY project with professional-grade reliability
- An open-source innovation platform for smart home developers

---

## Brand Personality

### Voice Attributes

| Attribute | Description | Example |
|-----------|-------------|---------|
| **Confident** | We know our technology is superior and speak with authority | "Our 4-state engine eliminates the 'twitchiness' that plagues other sensors." |
| **Technical** | We don't shy away from complexity—our audience appreciates it | "Using z-score statistical analysis and MAD-based calibration..." |
| **Accessible** | We translate complex concepts into user benefits | "This means your lights won't turn off while you're sleeping." |
| **Transparent** | We expose our reasoning and methodology openly | "Every decision is logged in the state reason sensor for full transparency." |
| **Innovative** | We emphasize our novel approaches and forward thinking | "Our innovative Absolute Clear Delay feature solves the age-old stillness problem." |

### Tone Guidelines

**DO:**
- Use active voice and strong verbs ("eliminates," "ensures," "delivers")
- Back claims with specific technical details
- Acknowledge and directly address user pain points
- Show confidence without arrogance
- Use technical terminology when speaking to technical audiences
- Explain complex features in benefit-oriented terms

**DON'T:**
- Use marketing hyperbole without substance ("revolutionary," "game-changing" without proof)
- Make vague promises ("better" without quantifying how)
- Oversimplify technical capabilities to the point of inaccuracy
- Talk down to readers or assume they lack technical knowledge
- Use emoji or casual language in technical documentation
- Avoid mentioning limitations or trade-offs

---

## Visual Identity

### Color Palette

**Primary Colors:**
- **Deep Navy** (#0A1929) - Authority, technical sophistication
- **Bright Cyan** (#00E5FF) - Innovation, digital intelligence
- **Pure White** (#FFFFFF) - Clarity, transparency

**Secondary Colors:**
- **Slate Gray** (#546E7A) - Technical diagrams, neutral elements
- **Amber Accent** (#FFC107) - Warnings, attention highlights
- **Success Green** (#4CAF50) - Validation, positive states

**Data Visualization:**
- **Signal High** (#00E5FF) - Above threshold, presence detected
- **Signal Normal** (#90CAF9) - Within normal range
- **Signal Low** (#1976D2) - Below threshold, absence

### Typography

**Headings:**
- Font: Inter, -apple-system, or similar modern sans-serif
- Weight: Semi-bold to Bold (600-700)
- Usage: Clear hierarchy, generous spacing

**Body Text:**
- Font: Inter, system-ui, or similar readable sans-serif
- Weight: Regular (400) for body, Medium (500) for emphasis
- Line height: 1.6-1.8 for readability

**Code/Technical:**
- Font: 'JetBrains Mono', 'Fira Code', monospace
- Usage: Code snippets, parameter names, entity IDs
- Style: Syntax highlighting where appropriate

### Iconography Style
- **Line-based icons** with 2px stroke weight
- Geometric precision (circles, squares, clean angles)
- Minimal detail, maximum clarity
- Consistent 24x24px base grid
- Example themes: Circuit patterns, waveforms, state diagrams

---

## Messaging Framework

### Core Value Propositions

1. **Unmatched Reliability**
   - **Claim:** "99.9% accuracy in real-world conditions"
   - **Proof:** 4-state machine with temporal debouncing
   - **Benefit:** "No more lights turning off while you sleep"

2. **Statistical Intelligence**
   - **Claim:** "Adapts to your unique environment automatically"
   - **Proof:** Z-score normalization with MAD-based calibration
   - **Benefit:** "Works reliably regardless of room layout or sensor placement"

3. **Privacy by Design**
   - **Claim:** "Zero compromise on privacy"
   - **Proof:** mmWave radar (not cameras) + on-device processing
   - **Benefit:** "Knows *that* you're there, not *who* you are"

4. **Eliminates Stillness Errors**
   - **Claim:** "Solves the 'perfectly still' failure mode"
   - **Proof:** Absolute Clear Delay algorithm
   - **Benefit:** "Detects sleeping people reliably, even in deep sleep"

5. **Full Transparency & Control**
   - **Claim:** "Every parameter is tunable, every decision is explained"
   - **Proof:** Real-time state reason sensors + runtime configuration
   - **Benefit:** "Tune it exactly to your needs and understand why it works"

6. **Immune to Environmental Noise**
   - **Claim:** "Ignores fans, pets, and ambient motion"
   - **Proof:** Focus on still energy + distance windowing
   - **Benefit:** "Only reacts to actual bed occupancy, nothing else"

### Competitive Differentiation

| Competitor | Their Weakness | Our Advantage |
|------------|----------------|---------------|
| **Aqara FP2** | Black box algorithms, limited tuning | Full transparency, every parameter exposed |
| **Pressure Mats** | Mechanical failure, uncomfortable | No physical contact, mmWave reliability |
| **Apollo/Everything Presence** | General-purpose motion detection | Purpose-built for bed presence with temporal filtering |
| **Camera-based** | Privacy concerns | Privacy-first mmWave radar |
| **Withings/SlumberTek** | Closed ecosystem, cloud-dependent | Open source, local processing, Home Assistant native |

---

## Audience Profiles

### Primary: Home Assistant Power Users

**Demographics:**
- Age: 25-45
- Technical proficiency: High
- Smart home investment: $2,000+ in devices
- Programming experience: Basic to advanced

**Pain Points:**
- "My PIR sensors turn off lights while I'm still in bed"
- "Pressure mats are unreliable and uncomfortable"
- "I can't tune my current sensor—it's a black box"
- "False positives from my cat are driving me crazy"

**Motivations:**
- Perfect automation execution
- Deep integration with existing Home Assistant setup
- Understanding and controlling system behavior
- Contributing to open-source community

**Communication Style:**
- Appreciates technical depth
- Values documentation quality
- Trusts data and test results over marketing claims
- Prefers GitHub repos over product pages

### Secondary: DIY Electronics Enthusiasts

**Demographics:**
- Age: 20-50
- Technical proficiency: Moderate to high
- Interest: ESP32 development, sensor projects
- Community: Maker spaces, electronics forums

**Pain Points:**
- "Most projects lack proper documentation"
- "Commercial solutions don't teach me anything"
- "I want to understand how it works, not just use it"

**Motivations:**
- Learning advanced embedded development
- Building custom smart home solutions
- Showcasing technical skills
- Modifying and extending projects

**Communication Style:**
- Values code quality and test coverage
- Appreciates architectural explanations
- Interested in design decisions and trade-offs
- Engages through GitHub issues and contributions

---

## Content Guidelines

### Technical Content

**State Machine Diagrams:**
- Always use ASCII/Unicode box-drawing characters for accessibility
- Label all states and transitions clearly
- Include timing information (3s, 5s, 30s)
- Show both conditions (z-score thresholds) and actions (binary sensor state)

**Code Examples:**
- Provide context before the code (what it does, why it matters)
- Use proper syntax highlighting
- Include comments for non-obvious logic
- Show expected output when relevant

**Parameter Documentation:**
- Format: `parameter_name` (default: X, range: Y-Z)
- Always explain units (milliseconds, centimeters, z-score)
- Provide tuning guidelines (increase for X effect, decrease for Y effect)
- Link to relevant troubleshooting sections

### User-Facing Content

**Feature Descriptions:**
- Lead with the benefit ("Prevents false triggers")
- Follow with the mechanism ("using a 3-second debounce timer")
- Provide a concrete example ("so your lights won't turn off when you briefly sit up")

**Getting Started Guides:**
- Assume basic Home Assistant knowledge
- Link to external resources for prerequisites
- Use numbered steps with clear success criteria
- Include troubleshooting tips inline
- Show screenshots of Home Assistant UI where helpful

**Troubleshooting Content:**
- Start with the symptom ("Sensor turns off while I'm sleeping")
- Explain the likely cause ("z-score dropping during stillness")
- Provide the solution ("Increase abs_clear_delay_ms to 60000")
- Include verification steps ("Check state reason sensor")

---

## Marketing Asset Guidelines

### Hero Images/Videos

**Requirements:**
- Show Home Assistant dashboard as primary visual
- Include live sensor data and state changes
- Demonstrate reliability (stable vs. twitchy comparison)
- Use dark theme for modern aesthetic
- Overlay explanatory text when necessary

**Comparison Videos:**
- Split screen: "Generic Sensor" vs. "Our Sensor"
- Same scenario (person moving in bed)
- Show generic sensor flapping ON/OFF
- Show our sensor remaining stable with state machine working

### State Machine Visualizations

**Diagram Standards:**
- Use the 4-state flow from the architecture docs
- Color code states: IDLE (gray), DEBOUNCING_ON (yellow), PRESENT (green), DEBOUNCING_OFF (orange)
- Show arrows with conditions
- Include timing indicators
- Make it interactive on web if possible

### Dashboard Screenshots

**Must Include:**
- Binary sensor status (large, prominent)
- Real-time sensor graphs (still energy, z-score)
- Tunable number sliders (k_on, k_off, timers)
- State reason text sensor
- Change reason text sensor
- Clean, organized layout

**Annotations:**
- Point out key features with arrows/callouts
- Explain what each element does
- Show "before and after" tuning states

---

## Channel-Specific Guidelines

### GitHub README
- Lead with status badges (build, tests)
- Technical elevator pitch in first paragraph
- Link to full documentation early
- Include quick start for developers
- Emphasize test coverage and code quality
- Use technical voice with confidence

### Product Landing Page
- Emotionally resonant hero section (pain point → solution)
- Feature breakdown with benefits first, technical details second
- Visual demonstrations (GIF/video)
- Social proof (GitHub stars, testimonials from users)
- Clear calls-to-action (View Docs, Get Started, See GitHub)
- Competitive comparison table

### Documentation Site
- Assumes user has already decided to use the product
- Highly technical, implementation-focused
- Extensive code examples and configuration samples
- Architecture deep-dives with diagrams
- Troubleshooting guides for every failure mode
- API reference with all parameters documented

### Social Media / Forums
- Share technical achievements (test coverage milestones, new features)
- Respond to questions with helpful detail, link to docs
- Showcase community contributions
- Behind-the-scenes development insights
- Avoid marketing speak, maintain authentic technical voice

---

## Brand Don'ts

❌ **Don't:** Use consumer-grade marketing language
✅ **Do:** Speak to technical sophistication of audience

❌ **Don't:** Promise features that aren't implemented
✅ **Do:** Be transparent about roadmap and limitations

❌ **Don't:** Oversimplify technical concepts to the point of inaccuracy
✅ **Do:** Explain complex ideas in accessible but accurate terms

❌ **Don't:** Compete on price
✅ **Do:** Compete on quality, reliability, and transparency

❌ **Don't:** Hide trade-offs or known issues
✅ **Do:** Document limitations and provide workarounds

❌ **Don't:** Use stock photos of generic smart homes
✅ **Do:** Show real dashboards, real code, real data

---

## Trademark & Legal

### Product Name Usage
- **Correct:** "Bed Presence Sensor" or "the bed presence sensor"
- **Incorrect:** "BedPresenceSensor" (except in code), "BPS", "Bed Sensor"

### Open Source Positioning
- Always mention Apache 2.0 license
- Encourage contributions and forks
- Credit community contributors prominently
- Link to GitHub repository liberally

### Claims & Validation
- Back all performance claims with test results
- Reference specific tests or validation procedures
- Don't make unverifiable claims about accuracy without data
- Update documentation when performance characteristics change

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-10 | Initial brand guidelines established |

---

## Contact & Governance

For questions about brand usage or to propose changes to these guidelines:
- **GitHub Issues:** [r-mccarty/bed-presence-sensor/issues](https://github.com/r-mccarty/bed-presence-sensor/issues)
- **Discussions:** GitHub Discussions (for community input)

These guidelines are maintained as living documentation and may evolve as the project matures.
