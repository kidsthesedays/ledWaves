# Scale Mapper

A 2D zigzag panel sketch built **on FastLED Hub** from scratch. One physical
WS2811/WS2812 strip wired in a serpentine (zigzag) is driven as an X/Y grid.
FastLEDHub provides the mobile web interface, animation switching, brightness
& speed sliders (with gamma correction), and automatic persistence of every
slider/color — so the panel geometry is tuned live from a phone and stored
across reboots.

## Mapping model

```
start pad → row 0 (left→right) → [U-turn gap dead px] → row 1 (right→left) → ... → end pad
```

- (0,0) is the **top-left corner** (standard graphics orientation),
  X grows right, Y grows down
- "Columns" — pixels per row, "Rows" — number of rows
- "U-Turn gap" — dead/unlit pixels consumed at every U-turn point
- "Start pad" / "End pad" — unlit pixels at the head/tail of the strip
- Physical position = `startPad + y·(cols + gap) + (even row ? x : cols−1−x)`

## Files

- `ScaleMapper.ino` — main sketch: LEDs, slider/color-picker registration,
  animation registration
- `Animations/XYMap.h` — the zigzag X/Y ⇄ strip-index mapping (`Panel`)
- `Animations/PanelAnimations.h` — all animations

## Animations

`Solid`, `Glow` (breathing radial blend of the two picked colors), `Ripple`
(expanding rings), `Wave X` (ripple travelling along the rows), `Wave Y`
(ripple travelling down the rows), `Fire` (2D noise on the heat palette),
`Rainbow`, `Test Pattern` (white scan + start/end pads blue + U-turn gaps dim
red, to verify wiring against the sliders).

## Setup

1. Install the ESP32 boards package in Arduino IDE as usual.
2. Install **FastLEDHub** from the Library Manager; it pulls the required
   dependencies (FastLED, ArduinoJson ≥ 7, ESPEssentials ≥ 2.1, LinkedList,
   PubSubClient, WebSockets, WiFiManager ≥ 2.0). On first use, also install
   **ESPEssentials** explicitly if the IDE does not resolve it.
3. Set `NUM_LEDS`, `LED_TYPE` and `LIGHTSTRIP_PIN` in `ScaleMapper.ino`.
4. Upload the sketch.
5. Upload the web interface files to flash (one-time):
   - copy FastLEDHub's library `data/` folder into the sketch folder and use
     the Arduino filesystem uploader (ESP32 Sketch Data Upload), **or**
   - flash, then upload each file of that `data/` folder at
     `http://<device-ip>/edit`.
6. On first boot the FastLEDHub/WiFiManager portal lets you pick WiFi; then
   open `http://<device-ip>` — control animations, brightness, speed,
   colors and all five geometry sliders from any phone on that network.

HTTP automation endpoints (e.g. `http://<ip>/begin?index=2`, `/stop`,
`/toggle`, `/pause`, `/restart`) are built into FastLEDHub, as are MQTT/Home
Assistant discovery, alarm/sunset fades, and WLED/DDP/Hyperion UDP streaming.

## Building on top

Next steps this folder is set up for: per-row lengths as an extra custom
slider set, flip X/Y sliders remapping the origin for rotated/mirrored
installs, and an XY-aware frame previewer that taps into the same `Panel`
mapping.
