# ESP32 WebSocket Client

A minimal WebSocket client firmware for ESP32 that receives LED control commands from p5.js sketches via a WebSocket server.

## Overview

This firmware connects to a WebSocket server (like the [p5js-simple WebSocket Server](../../server/websocket-server/)) and listens for commands to control an attached LED strip. It supports:

- **Multiple LED modes**: fire, rainbow, solid color, off
- **Direct pixel control**: Set individual or all pixel colors
- **Real-time updates**: Immediate LED updates from p5.js
- **Status reporting**: Sends device status back to the server

## Features

- WebSocket client implementation using Markus Sattler's WebSockets library
- FastLED for efficient LED strip control
- ArduinoJson for JSON message parsing
- Support for both Station mode (connect to WiFi) and Access Point mode (create own network)
- Fire simulation effect with configurable parameters
- Rainbow, solid color, and off modes

## Dependencies

### Arduino Libraries

1. **WiFi** (built-in with ESP32 Arduino core)
2. **WebSockets by Markus Sattler** - For WebSocket client functionality
3. **FastLED** - For LED strip control
4. **ArduinoJson by Benoit Blanchon** - For JSON message parsing

### Installation

1. Open Arduino IDE
2. Go to **Sketch > Include Library > Manage Libraries**
3. Install the following libraries:
   - Search for "WebSockets" and install "WebSockets by Markus Sattler"
   - Search for "FastLED" and install "FastLED by Daniel Garcia"
   - Search for "ArduinoJson" and install "ArduinoJson by Benoit Blanchon"

## Hardware Requirements

- **ESP32 Development Board** (any variant with WiFi)
- **WS2811/WS2812/WS2812B LED Strip** (NeoPixel compatible)
- **5V Power Supply** (2A minimum for 240 LEDs)
- **1000uF Capacitor** (recommended between V+ and GND near ESP32)
- **330-470 Ohm Resistor** (optional, between ESP32 GPIO and LED data line)

## Wiring

```
ESP32 Pin 5 (DATA_PIN) ────┬──── Data In (DIN) on LED strip
                         │
ESP32 5V ────────────────┴──── 5V on LED strip
                         │
ESP32 GND ─────────────────┴──── GND on LED strip
```

## Configuration

Edit the configuration section at the top of `websocket-client.ino`:

```cpp
// LED Configuration
#define DATA_PIN    5           // GPIO pin for LED data
#define LED_TYPE    WS2812B    // WS2811, WS2812, WS2812B
#define COLOR_ORDER GRB         // GRB, RGB, BRG, etc.
#define NUM_LEDS    240        // Number of LEDs in strip

#define BRIGHTNESS  96         // Default brightness (0-255)
#define FRAME_RATE   60         // Target frame rate

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
bool useAP = false;  // Set to true to use Access Point mode

// WebSocket Configuration
const char* websocket_host = "192.168.1.100";  // WebSocket server IP
const int websocket_port = 8080;               // WebSocket server port
const char* websocket_path = "/";              // WebSocket path
```

## Usage

### Option 1: Connect to Existing WiFi Network

1. Set `useAP = false`
2. Enter your WiFi SSID and password
3. Set `websocket_host` to the IP of your WebSocket server
4. Upload to ESP32

### Option 2: Create Access Point

1. Set `useAP = true`
2. Set `ssid` and `password` for your ESP32's Access Point
3. Connect your computer to the ESP32's Access Point
4. Set `websocket_host` to the ESP32's IP (usually 192.168.4.1)
5. Upload to ESP32

### Option 3: Connect to p5js-simple WebSocket Server

1. Start the WebSocket server:
   ```bash
   cd p5js-simple/server/websocket-server
   npm start
   ```
2. Set `websocket_host` to your computer's IP address
3. Upload to ESP32
4. Open a p5.js sketch in your browser

## Supported Commands

The firmware responds to the following JSON commands:

### Mode Commands

```json
// Set LED mode
{"type": "mode", "mode": "fire|rainbow|solid|off"}
```

### Color Commands

```json
// Set solid color
{"type": "color", "color": [255, 100, 0]}

// Set single pixel
{"type": "pixel", "index": 10, "color": [255, 0, 0]}

// Set all pixels
{"type": "pixels", "data": [255,0,0, 0,255,0, 0,0,255, ...], "count": 240}
```

### Brightness Command

```json
// Set brightness
{"type": "brightness", "value": 128}
```

### Fire Effect Parameters

```json
// Configure fire effect
{"type": "fireParams", "cooling": 55, "sparking": 120}
```

### Status Request

```json
// Request device status
{"type": "status"}

// Response:
{
  "type": "status",
  "leds": 240,
  "mode": "fire",
  "brightness": 96,
  "freeMemory": 12345,
  "uptime": 60
}
```

### Ping/Pong

```json
// Ping
{"type": "ping"}

// Pong (automatic response)
{"type": "pong"}
```

## Message Flow

1. **Connection**: ESP32 connects to WebSocket server
2. **Identification**: ESP32 sends identification message
   ```json
   {"type": "identify", "client": "esp32", "leds": 240, "firmware": "websocket-client", "version": "1.0"}
   ```
3. **Commands**: Server (or p5.js via server) sends commands
4. **Execution**: ESP32 executes commands and updates LEDs
5. **Status**: ESP32 can send status updates

## Troubleshooting

### WiFi Connection Issues

- **Cannot connect to WiFi**: Verify SSID and password
- **AP mode not working**: Try a different channel or simpler password
- **IP address issues**: Use `WiFi.localIP()` in setup to debug

### WebSocket Connection Issues

- **Connection refused**: Verify server is running and IP is correct
- **Connection timeout**: Check firewall settings, ensure server is accessible
- **Disconnections**: Increase reconnect interval or check WiFi signal

### LED Issues

- **LEDs not lighting up**: Check wiring (DATA_PIN, 5V, GND)
- **Wrong colors**: Verify COLOR_ORDER matches your LED strip
- **Flickering**: Add capacitor, check power supply

### Debugging

Enable serial monitor (115200 baud) to see debug messages:

```
=== ESP32 WebSocket Client ===
Compiled: [date] [time]

Initializing LED strip...
LED strip initialized

Connecting to WiFi...
WiFi connected!
IP Address: 192.168.1.100

Connecting to WebSocket server...
Host: 192.168.1.100
Port: 8080
WebSocket client started
[WebSocket] Connected!
[WebSocket] Message: {"type":"mode","mode":"fire"}
Mode set to: fire
```

## Performance

- **Frame Rate**: ~60 FPS (configurable)
- **LED Update Rate**: Depends on WiFi signal and server load
- **Memory Usage**: ~10-20KB (depends on NUM_LEDS)
- **Latency**: ~10-100ms (WiFi + WebSocket + processing)

## Customization

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

### Adding New Commands

Add a new case to the message handler:

```cpp
else if (type == "newCommand") {
  if (doc.containsKey("param")) {
    int param = doc["param"];
    // Handle command
  }
}
```

## Alternative: Direct ESP32 to p5.js

For development without a bridge server, you can run a WebSocket server directly on the ESP32:

1. Use the **WebServer** and **WebSocketsServer** libraries
2. The ESP32 hosts both the web interface and WebSocket server
3. p5.js connects directly to the ESP32's IP

See the [FireSticks ESP32 firmware](../../../FireSticks/software/esp32/FireSticks/) for an example of this approach.

## License

MIT License - Feel free to use, modify, and distribute.
