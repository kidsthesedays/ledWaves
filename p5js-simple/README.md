# p5js-simple - Decoupled LED Control Development

ILLUTRON Kunst Pa Kajan - Simplified p5.js Development Environment

## Purpose

This folder provides a **decoupled development environment** for prototyping LED control interfaces using p5.js, separate from the ESP32/Teensy firmware. This allows:

- Parallel development of UI/visualization (p5.js) and embedded firmware
- Testing p5.js sketches without hardware dependencies
- Multiple communication protocol options (WebSocket, Serial/JSON, etc.)
- Easy integration with existing FireSticks/LEDWave infrastructure

## Architecture Overview

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   p5.js Sketches     │────▶│   Communication      │────▶│   ESP32/Teensy       │
│   (Browser)          │     │   Bridge Server      │     │   Microcontrollers   │
│                     │     │   (Node.js/WebSocket) │     │                     │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
                          │
                          ▼
                    ┌─────────────────────┐
                    │   Alternative:       │
                    │   Direct Serial/USB  │
                    │   (JSON protocol)    │
                    └─────────────────────┘
```

## Folder Structure

```
p5js-simple/
├── sketches/              # p5.js sketches for LED control
│   ├── basic-control/      # Basic LED control interface
│   ├── pixel-mapper/      # Pixel mapping visualization
│   ├── fire-simulator/    # Fire effect simulator
│   └── video-sampler/     # Video to LED sampling
├── server/                # Communication bridge servers
│   ├── websocket-server/  # WebSocket server (Node.js)
│   └── serial-bridge/     # Serial-to-WebSocket bridge
├── esp32/                 # Minimal ESP32 firmware examples
│   ├── websocket-client/  # WebSocket client firmware
│   ├── serial-json/       # Serial JSON protocol firmware
│   └── i2c-slave/         # I2C slave firmware (low latency)
├── docs/                  # Documentation
│   └── protocols.md       # Communication protocols
└── README.md              # This file
```

## Communication Options

### 1. WebSocket (Recommended for Development)
- **Protocol**: JSON over WebSocket
- **Port**: 81 (ESP32) or configurable
- **Format**: `{"type": "pixels", "data": [r,g,b,r,g,b,...]}`
- **Pros**: Real-time, bidirectional, easy debugging
- **Cons**: Requires WiFi on ESP32

### 2. Serial/USB (For Direct Connection)
- **Protocol**: JSON over Serial
- **Baud**: 115200
- **Format**: `{"type": "pixels", "data": [r,g,b,...], "count": 240}`
- **Pros**: No WiFi needed, lower latency
- **Cons**: Tethered connection

### 3. I2C (For Wired Low-Latency)
- **Protocol**: Binary over I2C
- **Format**: Pre-defined binary structure
- **Pros**: Very low latency, no WiFi
- **Cons**: Short distance, more complex wiring

## Quick Start

### Option A: WebSocket Development (No Hardware)

1. Start the WebSocket server:
   ```bash
   cd p5js-simple/server/websocket-server
   npm install
   node server.js
   ```

2. Open a sketch in browser:
   ```bash
   # Open in browser (using any local server)
   python -m http.server 8000
   # Then open http://localhost:8000/sketches/basic-control/
   ```

3. The sketch will connect to `ws://localhost:8080` by default

### Option B: Serial Development

1. Upload serial firmware to ESP32:
   ```bash
   # Open in Arduino IDE
   p5js-simple/esp32/serial-json/serial-json.ino
   ```

2. Start the serial bridge:
   ```bash
   cd p5js-simple/server/serial-bridge
   npm install
   node bridge.js /dev/ttyUSB0
   ```

3. Open a sketch and set `useSerial: true` in config

## Development Workflow

1. **Prototype in p5.js**: Create and test visualization/control logic in sketches/
2. **Test Communication**: Use the bridge servers to test with real hardware
3. **Integrate**: Move tested code to main FireSticks project

## p5.js Sketch Template

```javascript
// Basic structure for all p5.js LED control sketches

let config = {
  numLEDs: 240,
  host: 'localhost',
  port: 8080,
  useWebSocket: true,
  useSerial: false
};

let socket;
let ledColors = [];

function setup() {
  createCanvas(800, 600);
  
  // Initialize LED colors
  for (let i = 0; i < config.numLEDs; i++) {
    ledColors.push(color(0, 0, 0));
  }
  
  // Connect to server
  if (config.useWebSocket) {
    socket = new WebSocket(`ws://${config.host}:${config.port}`);
    socket.onmessage = handleMessage;
  }
}

function draw() {
  // Draw LED preview
  drawLEDStrip(ledColors);
  
  // Update and send
  updateLEDs();
  if (socket && socket.readyState === WebSocket.OPEN) {
    sendToServer();
  }
}

function handleMessage(event) {
  // Handle messages from server/ESP32
}

function sendToServer() {
  // Send pixel data to server
  let pixels = ledColors.map(c => [
    red(c), green(c), blue(c)
  ]).flat();
  socket.send(JSON.stringify({
    type: 'pixels',
    data: pixels,
    count: config.numLEDs
  }));
}
```

## ESP32 Firmware Template

### WebSocket Client (Minimal)

```cpp
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <FastLED.h>

#define NUM_LEDS 240
CRGB leds[NUM_LEDS];

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_TEXT:
      // Parse JSON and update LEDs
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      if (doc.containsKey("pixels")) {
        JsonArray pixels = doc["pixels"];
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB(pixels[i*3], pixels[i*3+1], pixels[i*3+2]);
        }
        FastLED.show();
      }
      break;
  }
}

void setup() {
  FastLED.addLeds<WS2812, 5, GRB>(leds, NUM_LEDS);
  WiFi.begin("SSID", "PASSWORD");
  webSocket.begin("192.168.1.100", 8080, "/");
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
}
```

## Next Steps

1. Explore the `sketches/` folder for working examples
2. Check `server/` for bridge implementations
3. See `esp32/` for minimal firmware examples
4. Review `docs/protocols.md` for communication protocol details

## Related Projects

- [FireSticks](../FireSticks/) - Main LED control project
- [ESP32](../ESP32/) - Existing ESP32 firmware
- [Pi5js](../Pi5js/) - p5.js GUI library
