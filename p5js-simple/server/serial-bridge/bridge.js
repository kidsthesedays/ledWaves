/**
 * Serial to WebSocket Bridge
 * 
 * This bridge allows p5.js sketches to communicate with ESP32 devices
 * over serial (USB) connections. It translates between:
 * - WebSocket (for p5.js browser communication)
 * - Serial (for ESP32 communication)
 * 
 * Usage:
 *   node bridge.js /dev/ttyUSB0 [port]
 *   
 *   /dev/ttyUSB0 - Serial port for ESP32
 *   port - WebSocket port (default: 8080)
 */

const SerialPort = require('serialport');
const WebSocket = require('ws');
const http = require('http');
const express = require('express');
const path = require('path');

// Configuration
const DEFAULT_WS_PORT = 8080;
const DEFAULT_BAUD = 115200;
const MESSAGE_DELIMITER = '\n';

// Parse command line arguments
const args = process.argv.slice(2);

if (args.length < 1) {
  console.error('Usage: node bridge.js <serialPort> [wsPort]');
  console.error('Example: node bridge.js /dev/ttyUSB0 8080');
  process.exit(1);
}

const SERIAL_PORT = args[0];
const WS_PORT = args[1] ? parseInt(args[1]) : DEFAULT_WS_PORT;

// Create Express app for status
const app = express();
const server = http.createServer(app);

// ============================================
// Serial Port Setup
// ============================================

const serialPort = new SerialPort(SERIAL_PORT, {
  baudRate: DEFAULT_BAUD,
  autoOpen: false
});

// Buffer for incoming serial data
let serialBuffer = '';

// ============================================
// WebSocket Server Setup
// ============================================

const wss = new WebSocket.Server({ server });
const webSocketClients = new Set();

// ============================================
// Express HTTP Server
// ============================================

app.use(express.json());

app.get('/', (req, res) => {
  res.json({
    bridge: 'Serial to WebSocket Bridge',
    version: '1.0',
    serialPort: SERIAL_PORT,
    baudRate: DEFAULT_BAUD,
    webSocketPort: WS_PORT,
    serialConnected: serialPort.isOpen,
    webSocketClients: webSocketClients.size
  });
});

app.get('/status', (req, res) => {
  res.json({
    serial: {
      port: SERIAL_PORT,
      isOpen: serialPort.isOpen,
      baudRate: DEFAULT_BAUD
    },
    webSocket: {
      port: WS_PORT,
      clients: webSocketClients.size
    }
  });
});

// ============================================
// Serial Port Handlers
// ============================================

serialPort.on('open', () => {
  console.log(`Serial port ${SERIAL_PORT} opened at ${DEFAULT_BAUD} baud`);
  
  // Send identification to ESP32
  sendSerialMessage({
    type: 'identify',
    client: 'serial-bridge',
    version: '1.0',
    protocol: 'serial-json'
  });
});

serialPort.on('data', (data) => {
  // Append data to buffer
  serialBuffer += data.toString();
  
  // Split by delimiter
  let messages = serialBuffer.split(MESSAGE_DELIMITER);
  
  // Keep last incomplete message in buffer
  serialBuffer = messages.pop() || '';
  
  // Process complete messages
  messages.forEach(msg => {
    if (msg.trim()) {
      try {
        const data = JSON.parse(msg);
        console.log(`Serial RX: ${JSON.stringify(data).substring(0, 100)}`);
        
        // Broadcast to all WebSocket clients
        webSocketClients.forEach(ws => {
          if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify(data));
          }
        });
        
      } catch (err) {
        console.error('Error parsing serial message:', err);
        console.error('Message:', msg);
      }
    }
  });
});

serialPort.on('error', (err) => {
  console.error('Serial port error:', err);
  
  // Notify WebSocket clients
  broadcastWebSocket({
    type: 'serialError',
    error: err.message,
    timestamp: Date.now()
  });
});

serialPort.on('close', () => {
  console.log('Serial port closed');
  
  // Notify WebSocket clients
  broadcastWebSocket({
    type: 'serialDisconnected',
    timestamp: Date.now()
  });
});

// ============================================
// WebSocket Server Handlers
// ============================================

wss.on('connection', (ws, req) => {
  const ip = req.socket.remoteAddress;
  console.log(`WebSocket client connected from ${ip}`);
  webSocketClients.add(ws);
  
  // Send welcome message
  ws.send(JSON.stringify({
    type: 'connected',
    bridge: 'serial-bridge',
    version: '1.0',
    serialPort: SERIAL_PORT,
    serialConnected: serialPort.isOpen,
    timestamp: Date.now()
  }));
  
  // Handle messages from WebSocket
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log(`WebSocket RX: ${JSON.stringify(data).substring(0, 100)}`);
      
      // Send to serial port
      if (serialPort.isOpen) {
        sendSerialMessage(data);
      } else {
        ws.send(JSON.stringify({
          type: 'error',
          code: 503,
          message: 'Serial port not connected',
          timestamp: Date.now()
        }));
      }
      
    } catch (err) {
      console.error('Error parsing WebSocket message:', err);
      ws.send(JSON.stringify({
        type: 'error',
        code: 400,
        message: 'Invalid JSON message',
        timestamp: Date.now()
      }));
    }
  });
  
  ws.on('close', () => {
    console.log('WebSocket client disconnected');
    webSocketClients.delete(ws);
  });
  
  ws.on('error', (err) => {
    console.error('WebSocket error:', err);
    webSocketClients.delete(ws);
  });
});

// ============================================
// Helper Functions
// ============================================

/**
 * Send message to serial port
 */
function sendSerialMessage(message) {
  if (!serialPort.isOpen) {
    console.log('Serial port not open, cannot send message');
    return;
  }
  
  const json = JSON.stringify(message);
  const data = json + MESSAGE_DELIMITER;
  
  console.log(`Serial TX: ${json.substring(0, 100)}`);
  serialPort.write(data, (err) => {
    if (err) {
      console.error('Error writing to serial port:', err);
    }
  });
}

/**
 * Broadcast message to all WebSocket clients
 */
function broadcastWebSocket(message) {
  const data = JSON.stringify(message);
  webSocketClients.forEach(ws => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(data);
    }
  });
}

// ============================================
// Start Server
// ============================================

console.log('========================================');
console.log('  Serial to WebSocket Bridge');
console.log('  Version: 1.0');
console.log(`  Serial: ${SERIAL_PORT} @ ${DEFAULT_BAUD} baud`);
console.log(`  WebSocket: ws://localhost:${WS_PORT}`);
console.log('========================================');
console.log('');

// Open serial port
serialPort.open((err) => {
  if (err) {
    console.error('Error opening serial port:', err);
    console.log('');
    console.log('Available serial ports:');
    SerialPort.list((err, ports) => {
      if (!err) {
        ports.forEach(port => {
          console.log(`  - ${port.path} (${port.manufacturer || 'Unknown'})`);
        });
      }
    });
  } else {
    console.log('Serial port opened successfully');
  }
});

// Start HTTP server
server.listen(WS_PORT, () => {
  console.log(`HTTP/WebSocket server listening on port ${WS_PORT}`);
  console.log('');
  console.log('Endpoints:');
  console.log(`  http://localhost:${WS_PORT}/          - Bridge info`);
  console.log(`  http://localhost:${WS_PORT}/status     - Status`);
  console.log(`  ws://localhost:${WS_PORT}/           - WebSocket`);
  console.log('');
  console.log('Press Ctrl+C to stop');
  console.log('');
});

// Handle process termination
process.on('SIGINT', () => {
  console.log('\nShutting down bridge...');
  
  // Close serial port
  if (serialPort.isOpen) {
    serialPort.close();
  }
  
  // Close WebSocket server
  wss.close();
  server.close(() => {
    console.log('Bridge stopped');
    process.exit(0);
  });
  
  // Force exit after 2 seconds
  setTimeout(() => {
    console.log('Force exit');
    process.exit(1);
  }, 2000);
});
