# ESP32 Serial JSON Protocol

A minimal firmware for ESP32 that receives LED control commands via Serial (USB) using JSON protocol. Designed to work with the [Serial-to-WebSocket Bridge](../../server/serial-bridge/) for p5.js communication.

## Overview

This firmware implements a **Serial JSON protocol** for controlling LED strips. It:

- Listens for JSON commands on the Serial port
- Parses commands using ArduinoJson
- Controls LEDs using FastLED
- Sends acknowledgments and status updates back via Serial

## Key Features

- **Simple Protocol**: JSON messages terminated with newline (`\n`)
- **No WiFi Required**: Uses USB Serial connection
- **Low Latency**: Direct connection, no network overhead
- **Bidirectional**: Can send status and acknowledgments back
- **Extensible**: Easy to add new commands

## Dependencies

### Arduino Libraries

1. **FastLED** - For LED strip control
2. **ArduinoJson by Benoit Blanchon** - For JSON message parsing

### Installation

1. Open Arduino IDE
2. Go to **Sketch > Include Library > Manage Libraries**
3. Install:
   - "FastLED by Daniel Garcia"
   - "ArduinoJson by Benoit Blanchon"

## Hardware Requirements

- **ESP32 Development Board** (any variant)
- **WS2811/WS2812/WS2812B LED Strip** (NeoPixel compatible)
- **5V Power Supply** (2A minimum for 240 LEDs)
- **USB Cable** (for Serial communication and power)
- **1000uF Capacitor** (recommended between V+ and GND)
- **330-470 Ohm Resistor** (optional, between ESP32 GPIO and LED data line)

## Wiring

```
ESP32 Pin 5 (DATA_PIN) ────┬──── Data In (DIN) on LED strip
                         │
ESP32 5V ────────────────┴──── 5V on LED strip
                         │
ESP32 GND ─────────────────┴──── GND on LED strip

USB Cable ─────────────────┴──── Computer (for Serial communication)
```

## Configuration

Edit the configuration section at the top of `serial-json.ino`:

```cpp
// LED Configuration
#define DATA_PIN    5           // GPIO pin for LED data
#define LED_TYPE    WS2812B    // WS2811, WS2812, WS2812B
#define COLOR_ORDER GRB         // GRB, RGB, BRG, etc.
#define NUM_LEDS    240        // Number of LEDs in strip

#define BRIGHTNESS  96         // Default brightness (0-255)
#define FRAME_RATE   60         // Target frame rate

// Serial Configuration
#define SERIAL_BAUD   115200   // Serial baud rate
```

## Usage

### Option 1: Direct Serial Communication

1. Upload firmware to ESP32
2. Open Serial Monitor (115200 baud)
3. Send JSON commands directly

Example session:
```
=== ESP32 Serial JSON Client ===
Compiled: [date] [time]
Protocol: serial-json v1.0
Baud: 115200
LEDs: 240

Initializing LED strip...
LED strip initialized

Waiting for commands...

# Send command
{"type": "mode", "mode": "fire"}
Mode set to: fire
{"type":"ack","command":"mode","value":"fire"}

# Send pixel data
{"type": "pixels", "data": [255,0,0,0,255,0,0,0,255], "count": 3}
Updated 3 pixels starting at 0
{"type":"ack","command":"pixels","value":"3"}

# Request status
{"type": "status"}
{"type":"status","leds":240,"mode":"fire","brightness":96,"freeMemory":12345,"uptime":60}
```

### Option 2: With Serial-to-WebSocket Bridge

1. Start the bridge:
   ```bash
   cd p5js-simple/server/serial-bridge
   node bridge.js /dev/ttyUSB0
   ```
2. Upload firmware to ESP32
3. Open p5.js sketch in browser
4. p5.js connects to WebSocket bridge, which forwards to ESP32 via Serial

### Option 3: With Node.js Script

```javascript
const SerialPort = require('serialport');

const port = new SerialPort('/dev/ttyUSB0', { baudRate: 115200 });

// Send command
port.write('{"type": "mode", "mode": "rainbow"}\n', (err) => {
  if (err) console.error('Error:', err);
});

// Receive responses
port.on('data', (data) => {
  console.log('Received:', data.toString());
});
```

## Protocol Specification

### Message Format

- **Encoding**: UTF-8
- **Delimiter**: Newline (`\n`, ASCII 10)
- **Format**: JSON object with `type` field

```json
{"type": "command", "param1": value1, "param2": value2}\n
```

### Message Types

#### Connection Messages

```json
// Ready (ESP32 → Host)
{"type": "ready", "firmware": "serial-json", "version": "1.0", "leds": 240}

// Identify (ESP32 → Host)
{"type": "identify", "client": "esp32", "firmware": "serial-json", "version": "1.0", "leds": 240}

// Ping (Host → ESP32)
{"type": "ping"}

// Pong (ESP32 → Host)
{"type": "pong"}
```

#### LED Control Messages

```json
// Set mode (Host → ESP32)
{"type": "mode", "mode": "fire|rainbow|solid|off"}

// Set color (Host → ESP32)
{"type": "color", "color": [255, 100, 0]}

// Set brightness (Host → ESP32)
{"type": "brightness", "value": 128}

// Set all pixels (Host → ESP32)
{"type": "pixels", "data": [255,0,0, 0,255,0, ...], "count": 240}

// Set all pixels with offset (Host → ESP32)
{"type": "pixels", "data": [255,0,0, ...], "count": 100, "offset": 50}

// Set single pixel (Host → ESP32)
{"type": "pixel", "index": 10, "color": [255, 0, 0]}

// Set fire parameters (Host → ESP32)
{"type": "fireParams", "cooling": 55, "sparking": 120}
```

#### Status Messages

```json
// Status request (Host → ESP32)
{"type": "status"}

// Status response (ESP32 → Host)
{
  "type": "status",
  "leds": 240,
  "mode": "fire",
  "brightness": 96,
  "freeMemory": 12345,
  "uptime": 60
}
```

#### Acknowledgment Messages

```json
// Acknowledgment (ESP32 → Host)
{"type": "ack", "command": "mode", "value": "fire"}

// Error (ESP32 → Host)
{"type": "error", "message": "Invalid JSON", "timestamp": 1234567890}
```

## Supported Commands

| Command | Direction | Description | Example |
|---------|-----------|-------------|---------|
| `ping` | Both | Keep-alive / connection test | `{"type": "ping"}` |
| `identify` | ESP32 → Host | Device identification | `{"type": "identify", ...}` |
| `status` | Host → ESP32 | Request device status | `{"type": "status"}` |
| `mode` | Host → ESP32 | Set LED mode | `{"type": "mode", "mode": "fire"}` |
| `color` | Host → ESP32 | Set solid color | `{"type": "color", "color": [255,0,0]}` |
| `brightness` | Host → ESP32 | Set brightness | `{"type": "brightness", "value": 128}` |
| `pixels` | Host → ESP32 | Set all pixels | `{"type": "pixels", "data": [...], "count": 240}` |
| `pixel` | Host → ESP32 | Set single pixel | `{"type": "pixel", "index": 0, "color": [255,0,0]}` |
| `fireParams` | Host → ESP32 | Configure fire effect | `{"type": "fireParams", "cooling": 55, "sparking": 120}` |

## Message Flow

### Normal Operation

```
Host: {"type": "mode", "mode": "fire"}\n
ESP32: Mode set to: fire
       {"type": "ack", "command": "mode", "value": "fire"}\n
Host: {"type": "pixels", "data": [255,0,0,...], "count": 240}\n
ESP32: Updated 240 pixels starting at 0
       {"type": "ack", "command": "pixels", "value": "240"}\n
Host: {"type": "status"}\n
ESP32: {"type": "status", "leds": 240, "mode": "fire", ...}\n
```

### Error Handling

```
Host: {"type": "mode"}\n  (missing 'mode' field)

ESP32: JSON parse error: ...
       {"type": "error", "message": "mode: missing 'mode' field", ...}\n
Host: invalid json

ESP32: JSON parse error: InvalidInput
       {"type": "error", "message": "Invalid JSON: InvalidInput", ...}\n
```

## Performance

### Throughput

- **Baud Rate**: 115200 (configurable up to 2Mbps)
- **Theoretical Max**: ~11.5 KB/s
- **Practical**: ~8-10 KB/s (due to overhead)

### Message Size

- **Max Recommended**: ~4KB per message
- **Practical Limit**: Depends on available memory
- **For 240 LEDs**: 720 bytes (240 * 3 RGB values)

### Latency

- **Serial Transmission**: <1ms for small messages
- **Processing**: <1ms for simple commands
- **Total**: <10ms (Serial + processing)

### Memory Usage

- **StaticJsonDocument<1024>**: ~1KB for parsing
- **LED Buffer**: NUM_LEDS * 3 bytes (240 * 3 = 720 bytes)
- **Heat Array**: NUM_LEDS * 1 byte (for fire effect)
- **Total**: ~2-3KB (depends on NUM_LEDS)

## Troubleshooting

### Serial Connection Issues

- **Port not found**: Check if ESP32 is connected via USB
- **Permission denied**: On Linux, add user to dialout group
- **Wrong baud rate**: Ensure baud rate matches on both ends
- **Garbage characters**: Check baud rate and wiring

### LED Issues

- **LEDs not lighting up**: Check DATA_PIN wiring
- **Wrong colors**: Verify COLOR_ORDER matches your LED strip
- **Flickering**: Add capacitor, check power supply
- **Only first LED works**: Check data line wiring

### JSON Parsing Issues

- **Parse error**: Check JSON syntax, ensure proper escaping
- **Out of memory**: Reduce message size or increase StaticJsonDocument size
- **Missing fields**: Ensure all required fields are present

### Debugging

Enable Serial Monitor (115200 baud) to see debug messages:

```
=== ESP32 Serial JSON Client ===
Compiled: [date] [time]
Protocol: serial-json v1.0
Baud: 115200
LEDs: 240

Initializing LED strip...
LED strip initialized

Waiting for commands...
RX: {"type": "mode", "mode": "fire"}
Mode set to: fire
{"type":"ack","command":"mode","value":"fire"}
```

## Customization

### Adding New Commands

Add a new case to the message handler:

```cpp
else if (type == "newCommand") {
  if (doc.containsKey("param")) {
    int param = doc["param"];
    // Handle command
    sendAck("newCommand", param);
  } else {
    sendError("newCommand: missing 'param' field");
  }
}
```

### Adding New Effects

Add a new case to the mode switch in `loop()`:

```cpp
case 'n':  // new mode
  newEffect();
  break;
```

And implement the effect function:

```cpp
void newEffect() {
  // Your effect code here
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
  FastLED.show();
}
```

### Changing LED Configuration

Modify the LED defines at the top:

```cpp
#define DATA_PIN    13  // Use different GPIO
#define LED_TYPE    WS2811
#define COLOR_ORDER BRG
#define NUM_LEDS    100  // Different number of LEDs
```

## Comparison with WebSocket

| Feature | Serial JSON | WebSocket |
|---------|-------------|-----------|
| **Connection** | USB cable | WiFi |
| **Latency** | <10ms | 10-100ms |
| **Throughput** | ~10 KB/s | ~1-10 MB/s |
| **Reliability** | Very high | Medium (depends on WiFi) |
| **Setup** | Simple | Requires WiFi config |
| **Portability** | Tethered | Wireless |
| **Multiple Devices** | No (1 port per ESP32) | Yes (1 server, many ESP32s) |

## Use Cases

### Best for Serial JSON:
- Tethered development (ESP32 connected to computer)
- Low-latency applications
- Simple setups with single ESP32
- Testing without WiFi

### Best for WebSocket:
- Wireless control
- Multiple ESP32s
- Remote control
- Production installations

## License

MIT License - Feel free to use, modify, and distribute.
