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

## Troubleshooting builds

### `'digitalPinToGPIONumber' is not a type` / conflicting `uartSetPins`

This error comes from FastLED's ESP32 startup replacement (`src/platforms/arduino/esp32_lazy_serial0_main.cpp.hpp`) colliding with the pin-remap macros of ESP32 core 3.3.2–3.3.12 on remap boards. It is not caused by this sketch. Pick one of these fixes:

1. **Select the plain board** (most common cause): the remap macros only exist on boards like *Arduino Nano ESP32*. If your hardware is a generic ESP32 (Dev Module, ESP32-C3 Super Mini, S3, …), choose that board in *Tools > Board* and the error disappears.
2. **Downgrade the ESP32 core** to 3.2.x via *Boards Manager* (esp32 by Espressif Systems). The startup replacement only engages on core ≥ 3.3.2 and stock startup compiles fine. FastLED + FastLEDHub are well-proven on 3.2.x.
3. **Patch the library locally** (survives until the next FastLED update): in
   `~/Documents/Arduino/libraries/FastLED/src/platforms/arduino/esp32_lazy_serial0_main.cpp.hpp`, add directly above the `extern "C" FL_LINK_WEAK bool uartSetPins(...);` line:
   ```cpp
   #ifdef uartSetPins
   #undef uartSetPins
   #endif
   ```
   The core's macro was rewriting the declaration's parameter names into nonsense; undef-ing it restores the C declaration. The call site is already protected with parentheses.
4. Not recommended: keep the Nano ESP32 board and edit core `io_pin_remap.h` — it gets overwritten on every core update.

Upstream this is FastLED issue #4914 class of bugs (core `io_pin_remap.h` macros mangle FastLED's own declarations).

## Building on top

Next steps this folder is set up for: per-row lengths as an extra custom
slider set, flip X/Y sliders remapping the origin for rotated/mirrored
installs, and an XY-aware frame previewer that taps into the same `Panel`
mapping.
