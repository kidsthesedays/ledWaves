# Panel Mapper — zigzag LED panel

One sketch, one strip, one X/Y grid. A single WS2811/WS2812 strip wired in a
serpentine (zigzag) is addressed as an X/Y grid. All geometry lives on the ESP32
and is adjustable live from a phone.

## Mapping model

```
start pad → row 0 (left→right) → [turnGap dead px] → row 1 (right→left) → ... → end pad
```

- `cols` — pixels per row; a toggle lets **every row have its own length**
  (enter each row's pixel count individually in the UI)
- `rows` — number of rows
- `turnGap` — dead/unlit pixels consumed at every U-turn point
- `startPad` / `endPad` — unlit pixels at the head/tail of the physical strip
- Flip X / Flip Y checkboxes re-map the logical origin; default is standard
  graphics orientation with (0,0) at the top-left corner

Physical pixels used (shown live in the UI, warned if it exceeds `NUM_LEDS`)
= `startPad + Σ rowLen + (rows−1) × turnGap`.

## Usage

1. Compile & upload with Arduino IDE (boards: ESP32; libraries: FastLED,
   ArduinoJson, WebSockets by Markus Sattler — same as FireSticks).
2. Set `DATA_PIN` / `LED_TYPE` / `COLOR_ORDER` / `NUM_LEDS` at the top of
   `PanelMapper.ino` for your physical strip.
3. Connect to the AP **FireSticks-Panel** (password: `fire2026`), open
   <http://192.168.4.1>.
4. Sliders (rows, columns, U-turn dead pixels, start/end pad, brightness,
   speed) apply instantly, effect renders right on the preview canvas.
   Uncheck "Same length for every row" to give each row its own pixel count.
5. **Test run** mode scans every grid cell white and marks start/end pads blue
   and U-turn dead zones dim red, so you can verify wiring against the layout.
6. Tap any cell in the preview to paint it with the selected color (helps
   identify a physical position); "Clear paint" resets.
7. **Save to flash** persists the current layout (including per-row lengths
   and flips)/colors/effects in NVS; loaded automatically on boot.

## Effects

`Glow` (breathing radial), `Ripple` (expanding concentric rings), `Wave X`
(ripple travelling along the rows), `Wave Y` (ripple travelling down the
rows), `Fire` (2D noise on the FastLED heat palette), `Rainbow`, `Solid`,
`Test run`, `Off`.
