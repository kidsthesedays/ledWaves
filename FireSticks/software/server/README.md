# Fire Sticks Server

A Node.js server that acts as a middleman between the web interface and multiple ESP32 devices. This enables scalable control of multiple Fire Sticks with video mapping capabilities.

## Architecture

```
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│   Your Browser   │──────▶│   Node.js Server │──────▶│   ESP32 #1       │
└─────────────────┘       │   (localhost:3000)│       └────────┬────────┘
                           └────────┬────────┘                │
                                    │                         ▼
                                    │                ┌─────────────────┐
┌─────────────────┐       │                │   LED Strips     │
│   Browser 2      │──────▶│                └─────────────────┘
└─────────────────┘       │                         
                           │                ┌─────────────────┐
                                    │                │   ESP32 #2       │
                                    └──────────────▶│   192.168.1.x   │
                                                     └────────┬────────┘
                                                              │
                                                              ▼
                                                     ┌─────────────────┐
                                                     │   LED Strips     │
                                                     └─────────────────┘
```

## Features

- **WebSocket Server**: Manages connections from browsers and ESP32s
- **HTTP Server**: Hosts the web interface files
- **API Endpoints**: REST API for configuration
- **Message Routing**: Intelligently routes messages between clients
- **Multi-ESP32 Support**: Connect and control multiple ESP32 devices
- **Video Mapping**: Coordinates video sampling across multiple sticks
- **Auto-Discovery**: Automatically detects connected ESP32s

## Quick Start

### Prerequisites

- Node.js (v18 or later)
- npm or yarn

### Installation

```bash
# Navigate to the server directory
cd FireSticks/software/server

# Install dependencies
npm install

# Start the server
npm start
```

The server will start on port 3000 by default.

### Using a Different Port

```bash
# Start on port 8080
npm run start:dev  # Uses port 3001

# Or specify custom port
node server.js 8080
```

## Configuration

### Environment Variables

Create a `.env` file in the server directory:

```env
PORT=3000
NODE_ENV=development
```

### Command Line Arguments

```bash
# Start server on specific port
node server.js 4000

# Start server on default port (3000)
node server.js
```

## Usage

### 1. Start the Server

```bash
cd FireSticks/software/server
npm start
```

You should see:
```
Fire Sticks Server running on port 3000
Web interface: http://localhost:3000
WebSocket (Browser): ws://localhost:3000/ws/browser
WebSocket (ESP32): ws://localhost:3000/ws/esp32
```

### 2. Connect ESP32 Devices

#### Option A: ESP32 Connects to Server (Recommended for Multiple ESP32s)

Modify your ESP32 firmware to connect to the server instead of creating its own AP:

```cpp
// In FireSticks.ino, replace the WiFi setup:

// Instead of:
// WiFi.softAP(ssid, password);

// Use:
WiFi.begin("your-wifi-ssid", "your-wifi-password");
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
Serial.println("");
Serial.print("Connected to WiFi. IP: ");
Serial.println(WiFi.localIP());

// Connect to server WebSocket
webSocket.begin("192.168.1.100", 3000, "/ws/esp32");  // Replace with your server IP
```

#### Option B: Server Connects to ESP32 (For Existing Firmware)

If you're using the existing firmware that creates its own AP, you can configure the server to proxy connections.

### 3. Open Web Interface

Open your browser and navigate to:
```
http://localhost:3000
```

Or for the video mapping interface:
```
http://localhost:3000/video-mapping
```

## API Endpoints

### GET /api/status

Returns server status and connected clients:

```json
{
  "server": "running",
  "port": 3000,
  "browserClients": 2,
  "esp32Clients": [
    {
      "id": "12345678",
      "address": "192.168.1.101",
      "sticks": [1, 2, 3]
    }
  ],
  "videoMappings": {
    "sticks": [...],
    "videoSource": "camera",
    "autoUpdate": false,
    "updateInterval": 100
  }
}
```

### GET /api/sticks

Returns all stick configurations:

```json
{
  "configurations": [...],
  "esp32Sticks": [...]
}
```

### POST /api/sticks

Save a stick configuration:

```json
{
  "stickId": 1,
  "config": {
    "x": 100,
    "y": 200,
    "length": 300,
    "angle": 0,
    "ledsPerMeter": 240
  }
}
```

### GET /api/video-mappings

Returns current video mapping configuration.

### POST /api/video-mappings

Update video mapping configuration:

```json
{
  "sticks": [...],
  "videoSource": "camera",
  "autoUpdate": true,
  "updateInterval": 100
}
```

## WebSocket API

### Browser Connection

Connect to: `ws://localhost:3000/ws/browser`

#### Messages from Server to Browser

```json
// Connection confirmation
{
  "type": "connected",
  "clientId": "12345678",
  "esp32Count": 2,
  "videoMappings": {...},
  "sticks": [...]
}

// ESP32 connected
{
  "type": "esp32Update",
  "esp32Id": "abc123",
  "sticks": [1, 2, 3],
  "info": {...}
}

// ESP32 disconnected
{
  "type": "esp32Disconnected",
  "esp32Id": "abc123"
}

// Video mappings updated
{
  "type": "videoMappings",
  "data": {...}
}

// Message from ESP32
{
  "type": "esp32Message",
  "esp32Id": "abc123",
  "data": {...}
}
```

#### Messages from Browser to Server

```json
// Send command to specific ESP32
{
  "type": "esp32Command",
  "esp32Id": "abc123",
  "command": {
    "mode": "fire",
    "color": [255, 100, 0]
  }
}

// Send command to all ESP32s
{
  "type": "esp32Command",
  "command": {
    "mode": "fire"
  }
}

// Update video mappings
{
  "type": "videoMappings",
  "sticks": [...],
  "videoSource": "camera"
}

// Update stick configuration
{
  "type": "stickConfig",
  "stickId": 1,
  "config": {...}
}
```

### ESP32 Connection

Connect to: `ws://localhost:3000/ws/esp32`

#### Messages from Server to ESP32

```json
// Identification
{
  "type": "serverIdentify",
  "serverId": "fire-sticks-server",
  "esp32Id": "12345678"
}

// Forwarded command
{
  "mode": "fire",
  "color": [255, 100, 0]
}

// Stick configuration
{
  "type": "stickConfig",
  "stickId": 1,
  "config": {...}
}

// Video mapping data
{
  "type": "videoData",
  "stickId": 1,
  "pixels": [
    {"index": 0, "color": [255, 0, 0]},
    {"index": 1, "color": [0, 255, 0]}
  ]
}
```

#### Messages from ESP32 to Server

```json
// Identification
{
  "type": "identify",
  "info": {
    "name": "FireStick-1",
    "numLEDs": 240,
    "version": "1.0"
  },
  "sticks": [1, 2, 3]
}

// Status update
{
  "type": "status",
  "status": {
    "mode": "fire",
    "brightness": 96,
    "fps": 60
  }
}

// Stick configuration
{
  "type": "stickConfig",
  "sticks": [1, 2, 3]
}

// Sensor data
{
  "type": "sensor",
  "value": 512
}
```

## Running in Production

### Using PM2 (Recommended)

```bash
# Install PM2 globally
npm install -g pm2

# Start the server with PM2
pm2 start server.js --name "fire-sticks-server"

# View logs
pm2 logs fire-sticks-server

# Stop the server
pm2 stop fire-sticks-server

# Restart on changes
pm2 start server.js --name "fire-sticks-server" --watch
```

### Using Docker

Create a Dockerfile:

```dockerfile
FROM node:18-alpine
WORKDIR /app
COPY package*.json ./
RUN npm install
COPY . .
EXPOSE 3000
CMD ["node", "server.js"]
```

Build and run:

```bash
docker build -t fire-sticks-server .
docker run -p 3000:3000 fire-sticks-server
```

## Development

### Using Nodemon

```bash
# Install nodemon globally
npm install -g nodemon

# Run with auto-restart
nodemon server.js
```

### Testing

```bash
# Start server in test mode
NODE_ENV=test node server.js 3001

# Run tests (if available)
npm test
```

## Troubleshooting

### Server Won't Start

1. Check if port is available:
   ```bash
   lsof -i :3000
   ```

2. Try a different port:
   ```bash
   node server.js 3001
   ```

### ESP32 Won't Connect

1. Verify WiFi connection
2. Check server IP address
3. Verify WebSocket URL: `ws://<server-ip>:3000/ws/esp32`
4. Check firewall settings

### Browser Can't Connect

1. Verify server is running
2. Check browser console for errors
3. Verify WebSocket URL: `ws://localhost:3000/ws/browser`
4. Try refreshing the page

### Multiple ESP32s Not Working

1. Ensure each ESP32 has a unique ID
2. Check that all ESP32s are on the same network
3. Verify server can see all ESP32s in the status API

## Message Flow Examples

### Example 1: Browser Changes Mode

```
Browser → Server: {"mode": "fire"}
Server → All ESP32s: {"mode": "fire"}
```

### Example 2: Video Mapping Update

```
Browser → Server: {"type": "videoMappings", "sticks": [...]}
Server → All Browsers: {"type": "videoMappings", "data": {...}}
Browser samples video → Browser → Server: {"type": "videoData", "pixels": [...]}
Server → Specific ESP32: {"type": "videoData", "pixels": [...]}
```

### Example 3: ESP32 Status Update

```
ESP32 → Server: {"type": "status", "status": {...}}
Server → All Browsers: {"type": "esp32Message", "esp32Id": "...", "data": {...}}
```

## Performance Considerations

### For Many ESP32s

- Consider using a more robust WebSocket library like Socket.io
- Implement message batching for high-frequency updates
- Use compression for large messages

### For High-Resolution Video

- Reduce sampling rate
- Use lower resolution video
- Implement frame skipping

### For Many Clients

- Consider using Redis for pub/sub
- Implement rate limiting
- Use a load balancer for very high traffic

## Security

### HTTPS/WSS

For production, use HTTPS and WSS:

```javascript
const https = require('https');
const fs = require('fs');

const options = {
  key: fs.readFileSync('key.pem'),
  cert: fs.readFileSync('cert.pem')
};

const server = https.createServer(options, app);
```

### Authentication

Add basic authentication:

```javascript
const basicAuth = require('express-basic-auth');

app.use(basicAuth({
  users: { 'admin': 'password' },
  challenge: true
}));
```

## License

MIT License - See the main repository LICENSE file.
