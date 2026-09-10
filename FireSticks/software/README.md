# Fire Sticks - Software Documentation

## Overview

The Fire Sticks software system provides control and visualization for LED strips using ESP32 microcontrollers and a web-based interface. This documentation covers the software architecture, setup, and usage.

## Software Architecture

```
Fire Sticks Software System
├── Embedded (ESP32)
│   ├── FastLED Library
│   ├── WiFi/WebSocket Server
│   ├── Fire Simulation
│   └── JSON Message Handler
└── Web Interface
    ├── p5.js (Visualization)
    ├── p5.gui (Controls)
    ├── WebSocket Client
    └── Pixel Mapping
```

## Embedded Software (ESP32)

### Firmware Features

- **FastLED Integration**: Full control over WS2811/WS2812 LED strips
- **WebSocket Server**: Real-time bidirectional communication with web interface
- **HTTP Server**: Serves basic web pages for control
- **Fire Simulation**: Advanced fire effect algorithm
- **Multiple Modes**: Fire, Rainbow, Solid Color, Off
- **Direct Pixel Control**: Individual LED color setting
- **JSON Message Protocol**: Standardized communication format

### Firmware Structure

```
FireSticks.ino
├── LED Configuration
├── WiFi Configuration
├── WebSocket Server
├── Fire Simulation Algorithm
├── Mode Handlers
├── JSON Message Parser
└── Main Loop
```

### Message Protocol

The ESP32 accepts JSON-formatted messages over WebSocket. The following message types are supported:

#### Mode Control
```json
{ "mode": "fire" }
{ "mode": "rainbow" }
{ "mode": "solid" }
{ "mode": "off" }
```

#### Color Control
```json
{ "color": [255, 100, 0] }
```

#### Brightness Control
```json
{ "brightness": 128 }
```

#### Fire Effect Parameters
```json
{ "cooling": 55 }
{ "sparking": 120 }
```

#### Direct Pixel Control
```json
{ "pixel": { "index": 10, "color": [255, 0, 0] } }
```

#### Pattern Control
```json
{ "pattern": "fire" }
```

#### Initialization
```json
{ "type": "init", "mode": "fire", "color": [255, 100, 0], "brightness": 96 }
```

### Response Messages

The ESP32 sends the following messages to the client:

#### Connection Acknowledgment
```json
{ "type": "connected", "leds": 240, "mode": "fire" }
```

## Web Interface

### Features

- **Real-time Preview**: Visual representation of LED strip
- **Mode Selection**: Fire, Rainbow, Solid, Off
- **Color Picker**: Full RGB color selection
- **Parameter Controls**: Cooling, Sparking, Brightness, Speed
- **Direct Pixel Control**: Set individual LED colors
- **Connection Status**: Visual feedback for WebSocket connection
- **Responsive Design**: Works on desktop and mobile devices

### Web Interface Structure

```
software/web/
├── index.html          # Main HTML file
├── css/
│   └── style.css      # Styling for the interface
├── js/
│   └── app.js          # Main application JavaScript
└── lib/
    ├── p5.js           # p5.js library
    ├── p5.gui.js       # Modified p5.gui library
    ├── p5.dom.js       # p5.dom library
    ├── quicksettings.js # QuickSettings library
    └── dat.gui.js      # DAT.GUI library (fallback)
```

### p5.js Integration

The web interface uses p5.js for:
- **LED Strip Visualization**: Real-time preview of LED colors
- **Fire Simulation Preview**: Client-side fire effect simulation
- **Rainbow Effect Preview**: Client-side rainbow effect
- **Color Conversion**: HSB to RGB conversion utilities

### p5.gui Integration

The interface uses p5.gui for parameter controls:
- **Sliders**: For numeric parameters (cooling, sparking, brightness)
- **Color Pickers**: For RGB color selection
- **Dropdowns**: For mode selection
- **Magic Variables**: For controlling slider ranges

## Setup Instructions

### Prerequisites

#### Hardware
- ESP32 development board
- WS2811/WS2812 LED strip
- 5V power supply
- USB cable for programming

#### Software
- Arduino IDE or PlatformIO
- FastLED library
- WiFi library (included with ESP32 board support)
- WebSockets library
- ArduinoJson library

### Arduino IDE Setup

1. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to File > Preferences
   - Add board manager URL: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Go to Tools > Board > Boards Manager
   - Search for "esp32" and install

2. **Install Required Libraries**
   - Sketch > Include Library > Manage Libraries
   - Install: FastLED, WiFi, WebServer, WebSockets, ArduinoJson

3. **Upload Firmware**
   - Open FireSticks.ino
   - Select board: ESP32 Dev Module
   - Select port: COM port for your ESP32
   - Upload

### Web Interface Setup

1. **Host the Web Interface**
   - Copy the `software/web` directory to a web server
   - Or open `index.html` directly in a browser (for development)

2. **Connect to ESP32**
   - ESP32 creates a WiFi access point: "FireSticks-AP"
   - Password: "firesicks2024"
   - Connect your computer/phone to this network
   - Open the web interface in a browser

3. **Alternative Connection**
   - Connect ESP32 to your existing WiFi network
   - Update the SSID and password in the firmware
   - Access the web interface at the ESP32's IP address

## Usage

### Basic Operation

1. **Connect to ESP32**
   - Ensure your device is connected to the FireSticks-AP network
   - Open the web interface in a browser
   - Connection status should show "Connected"

2. **Select Mode**
   - Click on mode buttons or use the dropdown in the settings panel
   - Available modes: Fire, Rainbow, Solid, Off

3. **Adjust Parameters**
   - Use sliders to adjust:
     - **Cooling**: How quickly the fire cools down
     - **Sparking**: How often new sparks appear
     - **Brightness**: Overall LED brightness
     - **Speed**: Animation speed
     - **Scale**: Effect scale

4. **Change Colors**
   - Use the color picker in the settings panel
   - Or set individual pixel colors using the pixel control

### Advanced Features

#### Direct Pixel Control
1. Enter the pixel index (0-239)
2. Select a color using the color picker
3. Click "Set Pixel" to update that specific LED

#### Keyboard Shortcuts
- **F1**: Toggle WebSocket connection
- **F2**: Fire Mode
- **F3**: Rainbow Mode
- **F4**: Solid Mode
- **F5**: Off Mode

#### Multiple Sticks
The system can control multiple LED strips in a daisy-chain configuration:
1. Connect strips in series (data out to data in)
2. Connect power in parallel to all strips
3. Update `NUM_LEDS` in the firmware to match total LED count
4. The web interface will automatically adjust

## Configuration

### Firmware Configuration

Edit the following constants in `FireSticks.ino`:

```cpp
// LED Configuration
#define DATA_PIN    5
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    240

// WiFi Configuration
const char* ssid = "FireSticks-AP";
const char* password = "firesicks2024";

// Display Configuration
#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  60
```

### Web Interface Configuration

Edit `js/app.js` to change default settings:

```javascript
const CONFIG = {
  numLEDs: 240,
  numSticks: 1,
  ledsPerStick: 240,
  defaultHost: '192.168.4.1',
  defaultPort: 81,
  colors: {
    fire: [255, 100, 0],
    water: [0, 100, 255],
    forest: [0, 255, 0],
    electric: [100, 0, 255],
    pink: [255, 0, 255]
  }
};
```

## Customization

### Adding New Effects

To add a new LED effect:

1. **Add Mode to Firmware**
   - Add a new case in the `loop()` function
   - Create a new effect function
   - Add mode to message handler

2. **Add Mode to Web Interface**
   - Add button in HTML
   - Add mode to JavaScript `setMode()` function
   - Add preview function in `updateLEDPreview()`

Example:

```cpp
// In FireSticks.ino
void loop() {
  // ... existing code ...
  if (currentMode == "pulse") {
    pulseEffect();
  }
  // ...
}

void pulseEffect() {
  // Your custom effect code
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
}
```

```javascript
// In app.js
function setMode(mode) {
  state.currentMode = mode;
  if (previewSketch.p && previewSketch.p.instances[0]) {
    previewSketch.p.instances[0].params.mode = mode;
  }
  sendMessage({ mode: mode });
}

// Add to updateLEDPreview
case 'pulse':
  updatePulsePreview(p, colors, params);
  break;

function updatePulsePreview(p, colors, params) {
  // Your preview code
}
```

### Custom Color Palettes

Add custom color palettes to the firmware:

```cpp
// Define custom palette
CRGBPalette16 firePalette = {
  CRGB::Black,
  CRGB::Red,
  CRGB::Orange,
  CRGB::Yellow,
  CRGB::White
};

// Use in effects
void fireEffect() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(firePalette, heat[i]);
  }
}
```

### Custom Message Types

Add custom message handling for new features:

```cpp
// In webSocketEvent function
if (doc.containsKey("customCommand")) {
  String command = doc["customCommand"];
  handleCustomCommand(command);
}
```

## Troubleshooting

### Common Issues

#### WebSocket Connection Fails
- Check that ESP32 is powered on
- Verify WiFi connection to FireSticks-AP
- Check that the IP address is correct
- Ensure no firewall is blocking WebSocket connections

#### LEDs Not Responding
- Check data line connection
- Verify LED strip power
- Check that `NUM_LEDS` matches your strip length
- Test with a simple pattern first

#### Colors Are Wrong
- Check `COLOR_ORDER` in firmware
- Verify LED strip type (WS2811 vs WS2812)
- Some strips use different color orders (GRB, RGB, BRG)

#### Flickering LEDs
- Check power supply current rating
- Add capacitors (1000uF) near power connections
- Verify data line integrity
- Try lowering the brightness

### Debugging

#### Serial Monitor
- Open Serial Monitor in Arduino IDE
- Set baud rate to 115200
- View debug messages from ESP32

#### Browser Console
- Open browser developer tools (F12)
- Check Console tab for WebSocket messages
- View network traffic

#### LED Testing
- Upload a simple test sketch to verify LED operation
- Test with known-good firmware

## Performance Optimization

### LED Strip Performance

- **Frame Rate**: Adjust `FRAMES_PER_SECOND` for smoother animation
- **Brightness**: Lower brightness reduces power consumption
- **LED Count**: Reduce `NUM_LEDS` for faster updates
- **Effect Complexity**: Simpler effects run faster

### Web Interface Performance

- **Preview Quality**: Reduce preview LED count for better performance
- **Frame Rate**: Adjust p5.js frame rate
- **Animation**: Use `noLoop()` and `loop()` to control when to update

## Version History

- **v1.0** (Current): Initial Fire Sticks software release
- **v0.9**: Beta version with basic functionality
- **v0.8**: Alpha version, ported from Serpent Fragment

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This software is released under the MIT License. See the main README for details.
