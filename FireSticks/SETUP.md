# Fire Sticks - Setup Guide

Complete setup instructions for both **Direct ESP32 Mode** and **Local Server Mode**.

## Quick Start (Choose One)

### Option 1: Direct ESP32 Mode (Simplest)
**No server needed!** The ESP32 hosts everything.

```bash
# 1. Upload firmware to ESP32
# 2. Connect to FireSticks-AP WiFi
# 3. Open http://192.168.4.1 in browser
```

### Option 2: Local Server Mode (Scalable)
**For multiple ESP32s and advanced features.**

```bash
# 1. Start the server
cd FireSticks/software/server
npm install
npm start

# 2. Open http://localhost:3000 in browser
# 3. Connect ESP32s to the server
```

---

## Detailed Setup Instructions

## Option 1: Direct ESP32 Mode

This is the **simplest approach** where the ESP32 itself runs a web server and WebSocket server. Perfect for:
- Single ESP32 setups
- Quick testing and prototyping
- Portable installations
- No internet connection required

### Requirements

- **Hardware**
  - ESP32 development board (any variant)
  - WS2811 or WS2812 LED strip (1m, low density)
  - 5V power supply (2A minimum)
  - USB cable for programming

- **Software**
  - Arduino IDE
  - Required libraries (see below)

### Step 1: Install Arduino IDE

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Open Arduino IDE
   - Go to **File > Preferences**
   - Add this URL to "Additional Boards Manager URLs":
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Click OK
   - Go to **Tools > Board > Boards Manager**
   - Search for "esp32" and install the latest version

### Step 2: Install Required Libraries

1. Open **Sketch > Include Library > Manage Libraries**
2. Install the following libraries:
   - **FastLED** (by Daniel Garcia) - For LED strip control
   - **WiFi** (built-in with ESP32) - For WiFi connectivity
   - **WebServer** (built-in with ESP32) - For HTTP server
   - **WebSockets** by Markus Sattler - For WebSocket communication
   - **ArduinoJson** by Benoit Blanchon - For JSON message parsing

### Step 3: Upload Firmware

1. Open the firmware file:
   ```
   FireSticks/software/esp32/FireSticks.ino
   ```

2. Configure the firmware (optional):
   ```cpp
   // LED Configuration
   #define DATA_PIN    5      // Change if using different GPIO
   #define LED_TYPE    WS2811  // Or WS2812
   #define COLOR_ORDER GRB    // Check your LED strip
   #define NUM_LEDS    240     // Number of LEDs in your strip

   // WiFi Configuration
   const char* ssid = "FireSticks-AP";
   const char* password = "firesicks2024";

   // Display Configuration
   #define BRIGHTNESS          96
   #define FRAMES_PER_SECOND  60
   ```

3. Select your board and port:
   - **Tools > Board**: ESP32 Dev Module
   - **Tools > Port**: Select your ESP32's COM port

4. Click the **Upload** button (→)

5. Wait for the upload to complete. You should see "Done uploading" in the status bar.

### Step 4: Connect to ESP32 WiFi

1. The ESP32 will create a WiFi network:
   - **Network Name (SSID)**: `FireSticks-AP`
   - **Password**: `firesicks2024`

2. On your computer/phone/tablet:
   - Disconnect from your current WiFi
   - Connect to `FireSticks-AP`
   - Enter password: `firesicks2024`

3. Wait for connection to establish.

### Step 5: Open Web Interface

1. Open a web browser (Chrome, Firefox, Safari, Edge)
2. Navigate to: `http://192.168.4.1`
3. You should see the Fire Sticks control interface

### Step 6: Test Your Setup

1. Click the mode buttons to change LED effects
2. Use the sliders to adjust parameters
3. Try the Video Mapping interface (link at top of page)

---

## Option 2: Local Server Mode

This approach uses a **Node.js server** as a middleman, enabling:
- Multiple ESP32 control
- Advanced video mapping
- Better scalability
- Centralized management

### Requirements

- **Hardware**
  - One or more ESP32 devices
  - WS2811/WS2812 LED strips
  - Computer to run the server (Windows, Mac, Linux)
  - WiFi router (all devices must be on the same network)

- **Software**
  - Node.js (v18 or later)
  - npm or yarn
  - Arduino IDE (for ESP32 firmware)

### Step 1: Install Node.js

1. Download and install [Node.js](https://nodejs.org/) (LTS version recommended)
2. Verify installation:
   ```bash
   node --version
   npm --version
   ```

### Step 2: Set Up the Server

1. Navigate to the server directory:
   ```bash
   cd FireSticks/software/server
   ```

2. Install dependencies:
   ```bash
   npm install
   ```

3. Start the server:
   ```bash
   npm start
   ```
   Or with a custom port:
   ```bash
   node server.js 4000
   ```

4. You should see:
   ```
   Fire Sticks Server running on port 3000
   Web interface: http://localhost:3000
   WebSocket (Browser): ws://localhost:3000/ws/browser
   WebSocket (ESP32): ws://localhost:3000/ws/esp32
   ```

### Step 3: Configure ESP32 for Server Mode

You need to modify the ESP32 firmware to connect to your server instead of creating its own AP.

1. Open `FireSticks/software/esp32/FireSticks.ino`

2. Replace the WiFi setup section:
   ```cpp
   // OLD CODE - REMOVE THIS:
   // WiFi.softAP(ssid, password);
   
   // NEW CODE - ADD THIS:
   WiFi.begin("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
   Serial.println("");
   Serial.print("Connected to WiFi. IP: ");
   Serial.println(WiFi.localIP());
   
   // Connect to server WebSocket
   // Replace 192.168.1.100 with your server's IP
   webSocket.begin("192.168.1.100", 3000, "/ws/esp32");
   ```

3. Update the server IP in the code to match your computer's local IP:
   - On Windows: Run `ipconfig` and find your IPv4 address
   - On Mac/Linux: Run `ifconfig` or `ip a` and find your IP

4. Upload the modified firmware to your ESP32(s)

### Step 4: Connect ESP32s to Server

1. Power on your ESP32(s)
2. Check the Serial Monitor (Tools > Serial Monitor, 115200 baud)
3. You should see:
   ```
   Connecting to WiFi...
   Connected to WiFi. IP: 192.168.1.101
   WebSocket server started
   ```

4. On the server console, you should see:
   ```
   ESP32 connected: 12345678 from 192.168.1.101
   ```

### Step 5: Open Web Interface

1. On any device on the same network, open a browser
2. Navigate to: `http://<server-ip>:3000`
   (Replace `<server-ip>` with your computer's IP)

3. You should see the Fire Sticks interface with ESP32 connection status

### Step 6: Add Multiple ESP32s

1. Repeat Step 3 and Step 4 for each additional ESP32
2. Each ESP32 will connect to the server automatically
3. The server will track all connected ESP32s
4. You can control all ESP32s from the same interface

---

## Video Mapping Setup

### For Direct ESP32 Mode

1. Open the web interface at `http://192.168.4.1`
2. Click "Video Mapping" in the navigation
3. Allow camera access when prompted
4. Add sticks by Ctrl+Clicking (or long-press on mobile)
5. Position and resize sticks to match your physical setup
6. Click "Send to ESP32" to update the LEDs

### For Local Server Mode

1. Open the web interface at `http://<server-ip>:3000/video-mapping`
2. The server will coordinate video sampling across all connected ESP32s
3. Add sticks for each physical Fire Stick
4. The server will route pixel data to the appropriate ESP32

---

## Network Configuration

### Finding Your Server IP

- **Windows**: Open Command Prompt and run `ipconfig`
- **Mac**: Open Terminal and run `ifconfig | grep "inet "`
- **Linux**: Open Terminal and run `ip a | grep "inet "`

### Port Forwarding (For Remote Access)

If you want to access the server from outside your local network:

1. Configure port forwarding on your router
2. Forward external port (e.g., 3000) to your server's internal IP
3. Note your public IP address
4. Access from anywhere at `http://<public-ip>:3000`

**Note**: This exposes your server to the internet. Use with caution and consider security measures.

### Static IP for Server

To prevent your server's IP from changing:

- Configure a static IP on your computer
- Or set up DHCP reservation on your router
- This ensures the ESP32s can always find the server

---

## Troubleshooting

### ESP32 Won't Upload

1. Check USB cable connection
2. Try a different USB port
3. Press and hold the BOOT button on ESP32 while uploading
4. Check that the correct COM port is selected
5. Ensure you have the correct board selected (ESP32 Dev Module)

### WiFi Connection Fails

1. Verify SSID and password are correct
2. Check that you're in range of the router
3. Try restarting the ESP32
4. Check for WiFi interference (other devices, microwaves, etc.)

### Web Interface Won't Load

1. Verify the ESP32 is powered on
2. Check that you're connected to the correct WiFi network
3. Try refreshing the browser
4. Check the browser console (F12) for errors
5. Try a different browser

### LEDs Not Responding

1. Check that the LED strip is properly connected
2. Verify the DATA_PIN setting matches your wiring
3. Check that NUM_LEDS matches your strip length
4. Test with a simple pattern first
5. Check the Serial Monitor for error messages

### Server Won't Start

1. Check that Node.js is installed
2. Verify you ran `npm install`
3. Try a different port:
   ```bash
   node server.js 3001
   ```
4. Check for port conflicts:
   ```bash
   lsof -i :3000
   ```

### ESP32 Won't Connect to Server

1. Verify the server is running
2. Check that the ESP32 is on the same network
3. Verify the server IP in the ESP32 code
4. Check that the WebSocket port is correct
5. Check firewall settings on the server computer

---

## Hardware Setup

### Single Stick Wiring

```
ESP32
├── GPIO 5 ─────────────► LED Strip Data In
├── 5V ─────────────────► LED Strip 5V
├── GND ────────────────► LED Strip GND
└── (Optional) A0 ────────► Potentiometer
```

### Multiple Sticks (Daisy-Chain)

```
ESP32
├── GPIO 5 ─────────────► Stick 1 Data In
│                       │
├── 5V ────┬───────────► Stick 1 5V
│          ├───────────► Stick 2 5V
│          └───────────► Stick 3 5V
│                          
└── GND ──┬───────────► Stick 1 GND
           ├───────────► Stick 2 GND
           └───────────► Stick 3 GND

Stick 1 Data Out ─────────► Stick 2 Data In
Stick 2 Data Out ─────────► Stick 3 Data In
```

**Important**: Power (5V and GND) should be connected in **parallel** to all sticks. Data must be connected in **series** (daisy-chained).

### Power Requirements

| LED Count | Current Draw (max) | Recommended Power Supply |
|-----------|-------------------|-------------------------|
| 60 LEDs   | 3.6A              | 5V, 4A                 |
| 120 LEDs  | 7.2A              | 5V, 8A                 |
| 240 LEDs  | 14.4A             | 5V, 15A                |

For longer runs, add **power injection** at multiple points.

---

## Configuration Files

### ESP32 Configuration

Edit `FireSticks/software/esp32/FireSticks.ino`:

```cpp
// LED Configuration
#define DATA_PIN    5          // GPIO pin for data
#define LED_TYPE    WS2811     // WS2811 or WS2812
#define COLOR_ORDER GRB        // GRB, RGB, BRG, etc.
#define NUM_LEDS    240        // Number of LEDs

// WiFi Configuration (Direct Mode)
const char* ssid = "FireSticks-AP";
const char* password = "firesicks2024";

// Display Configuration
#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  60
```

### Server Configuration

Edit `FireSticks/software/server/server.js`:

```javascript
// Change default port
const DEFAULT_PORT = 3000;
```

Or use command line:
```bash
node server.js 4000
```

### Web Interface Configuration

Edit `FireSticks/software/web/js/app.js`:

```javascript
const CONFIG = {
  numLEDs: 240,           // Match your LED count
  defaultHost: '192.168.4.1',  // ESP32 IP (direct mode)
  defaultPort: 81,       // ESP32 WebSocket port
  // ...
};
```

---

## Best Practices

### For Reliable Operation

1. **Power Supply**: Use a power supply with at least 20% more current than your maximum draw
2. **Power Injection**: For strips longer than 2-3 meters, inject power at both ends
3. **Data Line**: Keep data lines as short as possible (< 1 meter)
4. **Grounding**: Ensure all components share a common ground
5. **Decoupling**: Add 1000uF capacitors near power connections

### For Performance

1. **Frame Rate**: Reduce FRAMES_PER_SECOND for complex effects
2. **Brightness**: Lower brightness reduces power consumption and heat
3. **LED Count**: Reduce NUM_LEDS for faster updates
4. **Effect Complexity**: Simpler effects run faster

### For Development

1. **Serial Monitor**: Always have the Serial Monitor open for debugging
2. **Browser Console**: Check the browser console (F12) for WebSocket errors
3. **Test Incrementally**: Test each feature as you add it
4. **Backup Configurations**: Save your stick configurations regularly

---

## Example Setups

### Setup 1: Single Stick (Direct Mode)

**Hardware:**
- 1x ESP32
- 1x 1m WS2812 LED strip (60 LEDs/m)
- 5V 4A power supply

**Software:**
- Upload firmware to ESP32
- Connect to FireSticks-AP
- Open http://192.168.4.1

**Use Case:**
- Quick testing
- Simple installations
- Portable demos

### Setup 2: Three Sticks (Direct Mode)

**Hardware:**
- 1x ESP32
- 3x 1m WS2812 LED strips
- 5V 10A power supply
- Power distribution board

**Wiring:**
- Daisy-chain data lines
- Parallel power connections

**Software:**
- Update NUM_LEDS to 720 (240 per strip)
- Upload firmware
- Connect to FireSticks-AP
- Use Video Mapping to configure each stick

**Use Case:**
- Small installations
- Low-resolution video display
- Interactive art pieces

### Setup 3: Multiple ESP32s (Server Mode)

**Hardware:**
- 3x ESP32
- 3x 1m WS2812 LED strips
- WiFi router
- Computer for server

**Software:**
- Start server on computer
- Configure each ESP32 to connect to server
- Open http://<server-ip>:3000
- Control all sticks from one interface

**Use Case:**
- Large installations
- Distributed LED control
- Complex video mapping
- Multi-user control

---

## Advanced Configuration

### Custom Modes

Add new LED effects to the firmware:

```cpp
// In FireSticks.ino
void loop() {
  if (currentMode == "pulse") {
    pulseEffect();
  }
  // ... other modes
}

void pulseEffect() {
  // Your custom effect
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
}
```

### Custom Messages

Add new message types for custom features:

```cpp
// In webSocketEvent function
if (doc.containsKey("customCommand")) {
  String command = doc["customCommand"];
  handleCustomCommand(command);
}
```

### Multiple Sticks per ESP32

Configure each ESP32 to control multiple physical sticks:

```cpp
// Define stick configurations
struct StickConfig {
  int startLED;
  int numLEDs;
  String name;
};

StickConfig sticks[] = {
  {0, 80, "Stick 1"},
  {80, 80, "Stick 2"},
  {160, 80, "Stick 3"}
};
```

---

## Support

### Getting Help

1. Check the **Troubleshooting** section above
2. Review the **README.md** files in each directory
3. Check the **Serial Monitor** for error messages
4. Check the **Browser Console** for WebSocket errors

### Reporting Issues

When reporting issues, please include:
- Your hardware setup (ESP32 model, LED strip type, etc.)
- Your software versions (Node.js, Arduino IDE, etc.)
- The exact steps to reproduce the issue
- Any error messages from Serial Monitor or Browser Console
- Screenshots if applicable

---

## License

This project is licensed under the MIT License. See the LICENSE file for details.

---

## Version History

- **v1.0** - Initial release with Direct and Server modes
- **v0.9** - Beta version with basic functionality
- **v0.8** - Alpha version, ported from Serpent Fragment
