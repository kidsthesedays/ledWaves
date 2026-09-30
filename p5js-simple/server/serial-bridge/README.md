# Serial to WebSocket Bridge

A bridge that enables communication between p5.js sketches (via WebSocket) and ESP32 microcontrollers (via Serial/USB).

## Purpose

This bridge solves the problem of **direct browser-to-serial communication** which is not possible due to browser security restrictions. It allows:

- p5.js sketches to send commands to ESP32 over serial
- ESP32 to send data back to p5.js sketches
- Development without WiFi on the ESP32
- Lower latency than WebSocket-over-WiFi

## Architecture

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   p5.js Sketch       │────▶│   Bridge Server      │────▶│   ESP32             │
│   (Browser)          │     │   (Node.js)          │     │   (Serial/USB)      │
│   WebSocket         │     │   WebSocket + Serial │     │   JSON Protocol     │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

## Quick Start

### 1. Install Dependencies

```bash
cd p5js-simple/server/serial-bridge
npm install
```

### 2. Find Your Serial Port

```bash
# List available serial ports
npm run list-ports
```

Example output:
```
/dev/ttyUSB0 
/dev/ttyACM0 ESP32
```

### 3. Start the Bridge

```bash
# Basic usage (auto-detect port)
node bridge.js /dev/ttyUSB0

# With custom WebSocket port
node bridge.js /dev/ttyUSB0 9000

# With nodemon for auto-restart
npm run dev /dev/ttyUSB0
```

### 4. Upload Firmware to ESP32

Upload the [Serial JSON firmware](../../esp32/serial-json/) to your ESP32.

### 5. Connect from p5.js

```javascript
let socket;

function setup() {
  createCanvas(800, 600);
  
  // Connect to bridge
  socket = new WebSocket('ws://localhost:8080');
  
  socket.onopen = () => {
    console.log('Connected to bridge');
  };
  
  socket.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Received:', data);
  };
}

function draw() {
  if (socket && socket.readyState === WebSocket.OPEN) {
    // Send command to ESP32
    socket.send(JSON.stringify({
      type: 'mode',
      mode: 'fire'
    }));
  }
}
```

## Usage

### From p5.js to ESP32

1. p5.js sends JSON message via WebSocket to bridge
2. Bridge receives message and forwards to serial port
3. ESP32 receives JSON message and processes it

### From ESP32 to p5.js

1. ESP32 sends JSON message via serial with newline delimiter
2. Bridge receives message and broadcasts to all WebSocket clients
3. p5.js receives JSON message

## Protocol

### Message Format

All messages are JSON objects with a `type` field, terminated with a newline (`\n`).

```json
{"type": "mode", "mode": "fire"}\n
{"type": "pixels", "data": [255,0,0, ...], "count": 240}\n
{"type": "status", "leds": 240, "freeMemory": 12345}\n
```

### Common Message Types

| Type | Direction | Description |
|------|-----------|-------------|
| `identify` | Both | Identify client type and version |
| `ping` | Both | Keep-alive / connection test |
| `pong` | Both | Response to ping |
| `mode` | p5.js → ESP32 | Set LED mode (fire, rainbow, solid, off) |
| `color` | p5.js → ESP32 | Set color (for solid mode) |
| `brightness` | p5.js → ESP32 | Set brightness (0-255) |
| `pixels` | p5.js → ESP32 | Set all pixel colors |
| `pixel` | p5.js → ESP32 | Set single pixel color |
| `status` | ESP32 → p5.js | Device status update |
| `error` | Both | Error message |

See [Protocols Documentation](../../docs/protocols.md) for complete message reference.

## Configuration

### Serial Port Settings

- **Baud Rate**: 115200 (default)
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### Command Line Arguments

```bash
# Required: serial port
node bridge.js /dev/ttyUSB0

# Optional: WebSocket port (default: 8080)
node bridge.js /dev/ttyUSB0 9000

# Optional: Custom baud rate (set in code, not CLI)
# Edit DEFAULT_BAUD in bridge.js
```

### Environment Variables

```bash
# Serial port
SERIAL_PORT=/dev/ttyUSB0

# WebSocket port
WS_PORT=9000

# Then run
node bridge.js
```

## Examples

### Example 1: Basic LED Control

**p5.js Sketch:**
```javascript
let socket;
let colorIndex = 0;

function setup() {
  createCanvas(800, 600);
  socket = new WebSocket('ws://localhost:8080');
  
  socket.onopen = () => {
    // Set mode to solid color
    socket.send(JSON.stringify({ type: 'mode', mode: 'solid' }));
  };
}

function draw() {
  if (socket && socket.readyState === WebSocket.OPEN) {
    // Cycle through colors
    colorIndex = (colorIndex + 1) % 256;
    const r = (sin(colorIndex * 0.024) * 127 + 128) * 2;
    const g = (sin(colorIndex * 0.024 + 2) * 127 + 128) * 2;
    const b = (sin(colorIndex * 0.024 + 4) * 127 + 128) * 2;
    
    socket.send(JSON.stringify({
      type: 'color',
      color: [r, g, b]
    }));
  }
}
```

**ESP32 (Serial JSON Firmware):**
```cpp
// In loop()
if (Serial.available()) {
  String message = Serial.readStringUntil('\n');
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, message);
  
  if (!error && doc.containsKey("type")) {
    String type = doc["type"];
    
    if (type == "color") {
      int r = doc["color"][0];
      int g = doc["color"][1];
      int b = doc["color"][2];
      fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
      FastLED.show();
    }
  }
}
```

### Example 2: Pixel Data

**p5.js Sketch:**
```javascript
let socket;
let pixels = [];

function setup() {
  createCanvas(800, 100);
  socket = new WebSocket('ws://localhost:8080');
  
  // Initialize pixels
  for (let i = 0; i < 240; i++) {
    pixels.push([0, 0, 0]);
  }
}

function draw() {
  // Create rainbow effect
  for (let i = 0; i < 240; i++) {
    const hue = map(i + frameCount, 0, 240, 0, 255);
    pixels[i] = [hue, 255, 255];
  }
  
  // Send to ESP32
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({
      type: 'pixels',
      data: pixels.flat(),
      count: 240
    }));
  }
}
```

**ESP32:**
```cpp
if (doc.containsKey("pixels")) {
  JsonArray data = doc["pixels"];
  int count = doc["count"];
  
  for (int i = 0; i < count && i < NUM_LEDS; i++) {
    leds[i] = CRGB(data[i*3], data[i*3+1], data[i*3+2]);
  }
  FastLED.show();
}
```

## Troubleshooting

### Common Issues

#### Serial Port Not Found

```
Error opening serial port: Error: No such file or directory
```

**Solutions:**
1. Check if the port exists: `npm run list-ports`
2. Ensure ESP32 is connected via USB
3. Check if another program is using the port
4. On Linux, ensure user has permission: `sudo usermod -a -G dialout $USER`

#### Permission Denied

```
Error opening serial port: Error: Access denied
```

**Solutions:**
- On Linux: Add user to dialout group and reboot
- On Mac: May need to install driver for ESP32
- On Windows: Check Device Manager for COM port

#### Connection Refused

```
WebSocket connection failed: Connection refused
```

**Solutions:**
1. Ensure bridge server is running
2. Check the port number
3. Check firewall settings

#### No Messages Received

**Solutions:**
1. Check WebSocket readyState before sending
2. Verify serial port is open (check bridge console)
3. Check baud rate matches on both ends
4. Verify ESP32 firmware is using the correct protocol

### Debugging Tips

1. **Enable verbose logging**: Edit `bridge.js` and add `console.log` statements
2. **Test serial connection**: Use `screen` or `minicom` to test serial directly
3. **Test WebSocket**: Use `wscat` to test WebSocket connection
4. **Check messages**: Use a serial monitor to see what ESP32 is sending

## Performance

### Throughput

- **Serial (115200 baud)**: ~11 KB/s theoretical, ~8-10 KB/s practical
- **WebSocket**: ~1-10 MB/s (limited by network)
- **Bridge overhead**: Minimal (JSON parsing + forwarding)

### Latency

- **Serial**: <1ms (for small messages)
- **WebSocket**: 1-10ms (local)
- **Total**: 1-20ms (serial + WebSocket + processing)

### Message Size Limits

- **Serial**: Limited by buffer size (~4KB recommended)
- **WebSocket**: ~16KB default (configurable)

For large pixel data, consider:
1. Sending in chunks
2. Using binary format (not JSON)
3. Compressing data

## Alternative: Direct Serial from Node.js

If you don't need browser access, you can use Node.js directly:

```javascript
const SerialPort = require('serialport');

const port = new SerialPort('/dev/ttyUSB0', { baudRate: 115200 });

port.on('data', (data) => {
  console.log('Received:', data.toString());
});

port.write('{"type": "mode", "mode": "fire"}\n', (err) => {
  if (err) console.error('Error:', err);
});
```

## License

MIT License - Feel free to use, modify, and distribute.
