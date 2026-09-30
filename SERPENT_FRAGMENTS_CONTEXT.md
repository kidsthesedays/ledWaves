# Serpent Fragments — Hardware & Software Development Summary

*This document preserves historical context from the Serpent Fragments project, which evolved into the current ledWaves/FireSticks work. It provides architectural philosophy and technical insights that inform the current development direction.*

---

## Project Overview

Serpent Fragments is a **modular LED installation** composed of long, narrow LED panels ("fragments") that can be physically rearranged. The software allows these panels to be positioned digitally, mapping each physical panel onto regions of an image, video, or generative canvas.

The **long-term goal** is to create a **distributed, modular architecture** where physical layout and digital composition are tightly linked.

**Key Insight**: Think of each panel as a **viewport into a larger texture** — exactly like UV mapping in 3D graphics. Each panel samples from a source image/video, and the extracted pixels are sent to the LEDs.

---

## Overall Hardware Philosophy

**Separate responsibilities across different devices** instead of trying to make one board do everything.

### Role Distribution

| Device | Role | Responsibilities | NOT Responsible For |
|--------|------|------------------|---------------------|
| **Raspberry Pi 5** | Orchestrator | Main computer, GUI host, web server, project storage, file loading, sending commands to controllers | Driving LEDs directly |
| **Teensy 4.1** | LED Renderer | FastLED execution, receives commands from Pi, outputs LED data with accurate timing | GUI, networking, file management |
| **ESP32 Nodes** | Distributed Processing | Remote LED panels, wireless sensors, motion detection, audio analysis, touch sensors, local animations, wireless communication | Central orchestration |

---

## Proposed Architecture

```
┌─────────────────┐
│   Browser        │  ← p5.js / Vue interface
│  (p5.js / Vue)   │
└────────┬────────┘
         │ WebSocket
         ▼
┌─────────────────┐
│ Raspberry Pi 5   │  ← Brain / Orchestrator
└────────┬────────┘
         │
    ┌────┴────┐
    ▼         ▼
┌─────────┐  ┌─────────┐
│ USB Serial│  │   WiFi   │
└────┬────┘  └────┬────┘
     │            │
     ▼            ▼
┌─────────┐  ┌─────────┐
│ Teensy   │  │ ESP32   │  ← Distributed workers
│ 4.1      │  │ Nodes   │
└────┬────┘  └────┬────┘
     │            │
     ▼            ▼
  LED strips    LEDs / Sensors
```

**The Raspberry Pi** becomes the brain.
**The Teensy** becomes the renderer.
**The ESP32s** become distributed workers.

---

## GUI Direction

### Evolution
- **Originally considered**: Processing
- **Current preference**: p5.js + Vue
- **Reason**: Modern browser interface, could become a web app running on Raspberry Pi

### Core Interface Concept

The interface consists of **draggable rectangles** where each rectangle represents one physical LED panel.

Each rectangle has:
- position
- size
- rotation
- orientation
- LED count
- pixel density

Each rectangle **samples pixels from a larger source image/video** (viewport/UV mapping model).

### Event-Driven Design

Every interaction becomes an event:
```
drag → update component → component updates state → mapping recalculated → LED output changes
```

Vue's reactivity **naturally matches this workflow**.

### Possible Data Model

Each panel:
```json
{
    "id": "panel-1",
    "position": {"x": 100, "y": 200},
    "width": 50,
    "height": 300,
    "rotation": 45,
    "orientation": "vertical",
    "ledCount": 240,
    "pixelDensity": 30,
    "sourceX": 0,
    "sourceY": 0,
    "sourceWidth": 100,
    "sourceHeight": 600
}
```

Eventually all panels become a **JSON document** (project file).

---

## Bubble Sort Metaphor

An interesting conceptual thread from the project's development:

Physical installation requires **constant optimization** — repeatedly moving panels, cables, electronics, and boxes. The user compared this to **Bubble Sort**.

This could become:
- A **design philosophy**: Objects naturally "bubble" into optimal arrangements
- An **interface interaction**: Drag-and-drop sorting/optimization
- An **installation methodology**: Iterative refinement process
- An **animation behavior**: LEDs that sort themselves

---

## LED Hardware Notes

### Current Hardware Stack
- **Microcontrollers**: Teensy 4.1, ESP32
- **LEDs**: WS2812 / SK6812 strips

### Recommended Wiring

```
Teensy Data
    ↓
330–470Ω resistor
    ↓
LED Data In (DIN)
```

**Across power lines:**
```
5V ────┬──── LED V+
       │
1000µF ║
       │
GND ───┴──── LED GND
```

### Component Purposes

| Component | Purpose |
|-----------|---------|
| **330–470Ω resistor** | Reduces ringing, protects first LED |
| **1000µF capacitor** | Smooths startup current, absorbs spikes |

---

## Capacitor Notes

### Suitable Capacitors for 5V Systems
- **1000µF 16V** (recommended)
- **1000µF 10V** (works)
- **1000µF 25V** (works — voltage rating is maximum, not exact)
- **680µF** (also works)

**Voltage rating** only indicates **maximum voltage** — higher is fine.

### Combining Capacitors

Can combine capacitors **in parallel** to achieve desired capacitance:
- 470µF + 470µF = 940µF
- 680µF + 330µF = 1010µF

---

## Resistor Notes

### Correct Resistor Value
- **330Ω–470Ω** (recommended for WS2812/WS2811 data lines)

### Not Suitable
- **4k7 (4700Ω)** — Too high, will cause signal issues

### Resistor Notation Guide
| Notation | Value |
|----------|-------|
| 4k7 | 4700Ω |
| 3R9 | 3.9Ω |
| 470R | 470Ω |
| 1k | 1000Ω |

---

## Power Supply Notes

### Common Ground is Essential

**All of these must share ground:**
- Raspberry Pi
- Teensy
- ESP32
- LED power supply

**Important distinction**: Earth ground ≠ signal ground

### USB Power Considerations

| USB Version | Typical Current | Suitability |
|-------------|-----------------|-------------|
| USB 2.0 | 500mA | Small LED systems only |
| USB 3.0 | 900mA | Small-medium LED systems |
| USB-C | Up to several amps | Depends on charger |

**For larger LED systems**: Use dedicated 5V power supplies.

---

## Troubleshooting: Strange Green LED Issue

### Symptom
Sometimes one green LED lights up unexpectedly.

### Likely Causes
- Floating data line
- Poor ground connection
- Unstable power
- Loose connector
- Noisy data line

### Recommendations
1. Add **330Ω resistor** on data line
2. Add **1000µF capacitor** across power
3. Ensure **good ground** connection
4. Use **short data cable**
5. Check all connectors

---

## Connector Discovery: Samtec

### Issue Encountered
Originally purchased: **Samtec SL-103-G-10**

**Problem**: Normal square male headers do **NOT** fit these connectors.

**Reason**: These are **precision machined sockets** that require **round machined male pins**.

### Resolution Options
1. Buy machined male headers
2. Return Samtec connectors
3. Redesign PCB

---

## Board Comparison

| Board | Pros | Cons | Best For |
|-------|------|------|----------|
| **RP2040** | Cheap, excellent PIO (Programmable IO), custom state machines | No wireless | Specialized LED control, custom protocols |
| **ESP32-WROOM-32U** | Good wireless, mature | Older architecture, limited RAM | Wireless LED control, distributed nodes |
| **ESP32-S3** | Wireless, USB OTG, better architecture, AI instructions | Newer, less mature | Advanced wireless applications |
| **Teensy 4.1** | Fast (600 MHz Cortex-M7), excellent FastLED support, USB Host + USB Device simultaneously, large RAM, deterministic timing | More expensive | **LED rendering (primary choice)** |
| **Raspberry Pi 5** | Full computer, GUI, storage, networking | Not real-time, no direct LED driving | **Orchestration, GUI, project management** |

### PIO vs RMT

| Feature | RP2040 PIO | ESP32 RMT |
|---------|------------|------------|
| **Type** | Programmable IO processors | Hardware peripheral |
| **Flexibility** | Very high (custom state machines) | Good (designed for LEDs) |
| **Ease of Use** | More complex | Easier for LEDs |
| **Precision** | Excellent | Excellent for LEDs |
| **Best For** | Custom protocols, complex timing | Addressable LEDs (WS2812, etc.) |

**Conclusion**: RMT is **easier** for standard LED control; PIO is **more flexible** for custom protocols.

---

## Current Software Direction

### AI-Assisted Development

Strong interest in **"vibe coding"** — using AI assistants for development.

Recently experimented with **GitHub Copilot** and was impressed by development speed.

Now considering using AI-assisted development for:
- Vue interface building
- p5.js sketch creation
- Rapid prototyping

### Immediate Next Goal

Build a **browser-based interface** where:

1. LED panels appear as **draggable rectangles**
2. Panels can be **resized and rotated**
3. Panels **sample regions from an image/video**
4. **Mapping updates live**
5. Raspberry Pi **receives layout**
6. Pi **sends mapping to Teensy**
7. Teensy **renders LEDs**
8. ESP32 nodes **may later receive distributed mappings**

### Interface Vision

The interface should feel more like a **creative composition tool** (similar to Figma, TouchDesigner, MadMapper, or a node editor) than a traditional configuration utility. It should support **rapid experimentation** and eventually become the **primary way to compose, arrange, and control** the Serpent Fragments installation.

---

## Communication Protocols

### ESP32 Wireless Options
- **WiFi** (standard)
- **UDP** (low overhead)
- **MQTT** (publish/subscribe)
- **ESP-NOW** (future possibility, direct ESP32-to-ESP32)

---

## Relationship to Current Project (ledWaves/FireSticks)

This Serpent Fragments context **informs the current ledWaves/FireSticks development**:

- The **p5js-simple** folder implements the browser-based interface concept
- The **FireSticks** project uses ESP32 with WebSocket/Serial communication
- The **Teensy** folder contains LEDWaveWiredProto1 (similar to Serpent Fragments renderer)
- The **modular architecture** philosophy continues with separate Pi/Teensy/ESP32 roles

### Evolution

| Aspect | Serpent Fragments | Current (ledWaves/FireSticks) |
|--------|-------------------|-------------------------------|
| Panels | Multiple fragments | Fire Sticks (simplified) |
| GUI | Planned (p5.js/Vue) | Implemented (p5.js) |
| Renderer | Teensy 4.1 | ESP32 + Teensy |
| Orchestrator | Raspberry Pi | Raspberry Pi / Node.js server |
| Focus | Modular panels | Fire simulation |

---

## Action Items (From Context)

1. ✅ **Create browser-based interface** — p5js-simple sketches started
2. ⏳ **Implement draggable panel system** — Next step for p5.js
3. ⏳ **Develop viewport/UV mapping** — Sample from source to LEDs
4. ⏳ **Connect Raspberry Pi orchestrator** — Server infrastructure exists
5. ⏳ **Integrate Teensy renderer** — Existing FastLED code
6. ⏳ **Add ESP32 distributed nodes** — WebSocket/Serial firmware exists

---

## References

- [p5js-simple/README.md](./p5js-simple/README.md) — Current decoupled development setup
- [p5js-simple/docs/protocols.md](./p5js-simple/docs/protocols.md) — Communication protocol specifications
- [FireSticks/README.md](./FireSticks/README.md) — Current project documentation
- [FireSticks/SETUP.md](./FireSticks/SETUP.md) — Setup instructions
- [Teensy/LEDWaveWiredProto1/LEDWaveWiredProto1.ino](./Teensy/LEDWaveWiredProto1/LEDWaveWiredProto1.ino) — LED renderer example

---

*Document created from ChatGPT summary. Preserved for historical context and architectural reference.*
