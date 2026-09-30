# WebSocket Server

A simple WebSocket server for enabling communication between p5.js sketches and ESP32 microcontrollers.

## Purpose

This server acts as a **bridge** between:
- **p5.js sketches** running in a browser
- **ESP32 microcontrollers** running WebSocket client firmware

It allows you to:
- Develop p5.js LED control interfaces without hardware
- Test communication logic independently
- Connect multiple ESP32s to a single control interface
- Debug WebSocket communication easily

## Quick Start

### 1. Install Dependencies

```bash
cd p5js-simple/server/websocket-server
npm install
```

### 2. Start the Server

```bash
# Default port 8080
npm start

# Or specify a port
node server.js 9000

# Or with nodemon for auto-restart
npm run dev
```

### 3. Connect Clients

The server will start and display:
```
========================================
  p5js-simple WebSocket Server
  Version: 1.0
  HTTP: http://0.0.0.0:8080
  WebSocket: ws://0.0.0.0:8080
========================================
```

## Usage

### From p5.js Sketch

```javascript
let socket;

function setup() {
  createCanvas(800, 600);
  
  // Connect to server
  socket = new WebSocket('ws://localhost:8080');
  
  socket.onopen = () => {
    console.log('Connected to server');
    
    // Identify yourself
    socket.send(JSON.stringify({
      type: 'identify',
      client: 'p5js',
      name: 'My LED Control Sketch'
    }));
  };
  
  socket.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Received:', data);
    
    switch (data.type) {
      case 'connected':
        console.log('Connected with ID:', data.clientId);
        break;
      case 'pong':
        console.log('Pong received');
        break;
      case 'pixels':
        // Handle pixel data from ESP32
        break;
    }
  };
  
  socket.onclose = () => {
    console.log('Disconnected from server');
  };
}

function draw() {
  // Send pixel data
  if (socket && socket.readyState === WebSocket.OPEN) {
    const pixels = [255, 0, 0, 0, 255, 0, 0, 0, 255]; // RGB values
    socket.send(JSON.stringify({
      type: 'pixels',
      data: pixels,
      count: 3
    }));
  }
}
```

### From ESP32 (Arduino)

See the [ESP32 WebSocket Client](../esp32/websocket-client/) firmware.

## API Reference

### HTTP Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Server information |
| GET | `/status` | List of connected clients |

### WebSocket Messages

#### Server to Client

```json
// Connection confirmation
{"type": "connected", "server": "p5js-simple", "version": "1.0", "clientId": "client-0"}

// Response to ping
{"type": "pong", "timestamp": 1234567890, "from": "server"}

// Client connected notification
{"type": "clientConnected", "clientId": "client-1", "clientType": "esp32"}

// Client disconnected notification
{"type": "clientDisconnected", "clientId": "client-1"}

// List of all clients
{"type": "clients", "clients": [{"id": "client-0", "type": "p5js"}, ...]}

// Server shutdown notification
{"type": "serverShutdown", "timestamp": 1234567890}

// Error message
{"type": "error", "code": 400, "message": "Invalid JSON", "timestamp": 1234567890}
```

#### Client to Server

```json
// Identify client type
{"type": "identify", "client": "p5js|esp32", "name": "My Client", "info": {}}

// Ping
{"type": "ping"}

// List connected clients
{"type": "listClients"}

// Disconnect request
{"type": "disconnect"}

// Any other message type will be broadcast to all other clients
{"type": "pixels", "data": [255,0,0, ...], "count": 240}
{"type": "mode", "mode": "fire"}
{"type": "color", "color": [255, 100, 0]}
```

## Configuration

### Environment Variables

```bash
# Port
PORT=9000 node server.js

# Host
HOST=192.168.1.100 node server.js
```

### Command Line Arguments

```bash
# Port only
node server.js 9000

# Host and port
node server.js 9000 192.168.1.100
```

## Examples

### Example 1: p5.js controlling ESP32

1. Start the server: `node server.js`
2. Upload WebSocket client firmware to ESP32
3. Open p5.js sketch in browser
4. p5.js sends: `{"type": "mode", "mode": "fire"}`
5. Server broadcasts to ESP32
6. ESP32 receives and changes mode

### Example 2: Multiple ESP32s

1. Start the server
2. Upload firmware to multiple ESP32s
3. All ESP32s connect to server
4. p5.js sends pixel data once
5. Server broadcasts to all ESP32s
6. All ESP32s update simultaneously

### Example 3: Testing without Hardware

1. Start the server
2. Open two browser tabs with p5.js sketches
3. Sketches connect to server
4. Messages from one sketch are received by the other
5. Test communication logic without ESP32

## Troubleshooting

### Connection Issues

- **Error: Connection refused** - Server not running or wrong port
- **Error: WebSocket not supported** - Browser doesn't support WebSocket (unlikely)
- **Error: CORS** - Ensure your p5.js sketch is served from a web server, not file://

### Message Issues

- **Messages not received** - Check WebSocket readyState before sending
- **JSON parse errors** - Ensure all messages are valid JSON
- **Large messages** - WebSocket has message size limits (default ~16KB)

### Debugging

Enable debug logging:

```bash
DEBUG=websocket node server.js
```

Or add logging to the server code.

## Performance

- **Max connections**: Limited by system resources (typically 100s-1000s)
- **Message size limit**: ~16KB by default (configurable)
- **Throughput**: Depends on hardware, typically 1-10 Mbps
- **Latency**: Typically <10ms for local connections

## Security

This server is designed for **development use only** and has no authentication. For production use:

1. Add authentication (token-based or username/password)
2. Use HTTPS/WSS for encrypted connections
3. Implement rate limiting
4. Validate all incoming messages
5. Run behind a firewall

## License

MIT License - Feel free to use, modify, and distribute.
