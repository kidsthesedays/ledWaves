# Panel Mapper — zigzag LED panel

One sketch, one strip, one X/Y grid. A single WS2811/WS2812 strip wired in a
serpentine (zigzag) is addressed as an X/Y grid. All geometry lives on the ESP32
and is adjustable live from a phone.

## Mapping model

```
start pad → row 0 (cols px, left→right) → [turnGap dead px] → row 1 (right→left) → ... → end pad
```

- `cols` — pixels in each row
- `rows` — number of rows
- `turnGap` — dead/unlit pixels consumed at every U-turn point
- `startPad` / `endPad` — unlit pixels at the head/tail of the physical strip
- Even rows run left→right, odd rows run right→left

Physical pixels used = `startPad + rows × cols + (rows−1) × turnGap` (shown live in the UI and warned if it exceeds `NUM_LEDS`).

## Usage

1. Compile & upload with Arduino IDE (boards: ESP32; libraries: FastLED,
   ArduinoJson, WebSockets by Markus Sattler — same as FireSticks).
2. Set `DATA_PIN` / `LED_TYPE` / `COLOR_ORDER` / `NUM_LEDS` at the top of
   `PanelMapper.ino` for your physical strip.
3. Connect to the AP **FireSticks-Panel** (password: `fire2026`), open
   <http://192.168.4.1>.
4. Sliders (columns, rows, U-turn dead pixels, start/end pad, brightness,
   speed) apply instantly, effect renders right on the preview canvas.
5. **Test run** mode scans every grid cell white and marks start/end pads blue
   and U-turn dead zones dim red, so you can verify wiring against the layout.
6. Tap any cell in the preview to paint it with the selected color (helps
   identify a physical position); "Clear paint" resets.
7. **Save to flash** persists the current layout/colors/effects in NVS; loaded
   automatically on boot.

## Effects

`Glow` (breathing radial), `Ripple` (expanding rings), `Fire` (2D noise on the
FastLED heat palette), `Rainbow`, `Solid`, `Test run`, `Off`.
