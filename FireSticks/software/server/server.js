/**
 * Fire Sticks - Local Server
 * 
 * This server acts as a middleman between the web interface and multiple ESP32 devices.
 * It hosts the web interface and manages WebSocket connections to ESP32s.
 * 
 * Usage:
 *   node server.js [port]
 *   Default port: 3000
 */

const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');
const fs = require('fs');

// Configuration
const DEFAULT_PORT = 3000;
const ESP32_WS_PORT = 81; // ESP32 WebSocket port

// Parse command line arguments
const args = process.argv.slice(2);
const PORT = args[0] ? parseInt(args[0]) : DEFAULT_PORT;

// Create Express app and HTTP server
const app = express();
const server = http.createServer(app);

// Create WebSocket server for browser connections
const wssBrowser = new WebSocket.Server({ server, path: '/ws/browser' });

// Create WebSocket server for ESP32 connections (if ESP32s connect to this server)
const wssESP32 = new WebSocket.Server({ server, path: '/ws/esp32' });

// Store connected clients
const browserClients = new Map(); // browserClientId -> WebSocket
const esp32Clients = new Map(); // esp32Id -> { WebSocket, info }

// Store stick configurations
const stickConfigurations = new Map(); // stickId -> configuration

// Store video mapping configurations
const videoMappings = {
  sticks: [],
  videoSource: 'camera',
  autoUpdate: false,
  updateInterval: 100
};

// ============================================
// Express HTTP Server - Serve static files
// ============================================

app.use(express.static(path.join(__dirname, '../web')));
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// API endpoints
app.get('/api/status', (req, res) => {
  res.json({
    server: 'running',
    port: PORT,
    browserClients: browserClients.size,
    esp32Clients: Array.from(esp32Clients.values()).map(c => ({
      id: c.id,
      address: c.socket._socket.remoteAddress,
      sticks: c.sticks || []
    })),
    videoMappings: videoMappings
  });
});

app.get('/api/sticks', (req, res) => {
  res.json({
    configurations: Array.from(stickConfigurations.values()),
    esp32Sticks: Array.from(esp32Clients.values()).flatMap(c => c.sticks || [])
  });
});

app.post('/api/sticks', (req, res) => {
  const { stickId, config } = req.body;
  if (stickId && config) {
    stickConfigurations.set(stickId, config);
    // Broadcast to all ESP32s
    broadcastToESP32s({ type: 'stickConfig', stickId, config });
    res.json({ success: true });
  } else {
    res.status(400).json({ error: 'Missing stickId or config' });
  }
});

app.get('/api/video-mappings', (req, res) => {
  res.json(videoMappings);
});

app.post('/api/video-mappings', (req, res) => {
  Object.assign(videoMappings, req.body);
  // Broadcast to all browsers
  broadcastToBrowsers({ type: 'videoMappings', data: videoMappings });
  res.json({ success: true, mappings: videoMappings });
});

// Serve main page
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../web/index.html'));
});

// Serve video mapping page
app.get('/video-mapping', (req, res) => {
  res.sendFile(path.join(__dirname, '../web/video-mapping.html'));
});

// ============================================
// WebSocket Server - Browser Connections
// ============================================

wssBrowser.on('connection', (ws, req) => {
  const clientId = Date.now().toString();
  const ip = req.socket.remoteAddress;
  
  console.log(`Browser connected: ${clientId} from ${ip}`);
  browserClients.set(clientId, ws);
  
  // Send initial state
  ws.send(JSON.stringify({
    type: 'connected',
    clientId: clientId,
    esp32Count: esp32Clients.size,
    videoMappings: videoMappings,
    sticks: Array.from(stickConfigurations.values())
  }));
  
  // Handle messages from browser
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log(`Browser ${clientId} message:`, data);
      
      switch (data.type) {
        case 'identify':
          // Browser identifies itself
          break;
          
        case 'esp32Command':
          // Forward command to specific ESP32 or all
          if (data.esp32Id) {
            const esp32 = esp32Clients.get(data.esp32Id);
            if (esp32) {
              esp32.socket.send(JSON.stringify(data.command));
            }
          } else {
            // Broadcast to all ESP32s
            broadcastToESP32s(data.command);
          }
          break;
          
        case 'videoMappings':
          // Update video mappings
          Object.assign(videoMappings, data);
          broadcastToBrowsers({ type: 'videoMappings', data: videoMappings });
          break;
          
        case 'stickConfig':
          // Save stick configuration
          if (data.stickId) {
            stickConfigurations.set(data.stickId, data.config);
          }
          broadcastToESP32s(data);
          break;
          
        case 'sampleVideo':
          // Request to sample video and send to ESP32s
          // This would be handled by the browser's video mapper
          break;
          
        default:
          // Forward to all ESP32s
          broadcastToESP32s(data);
      }
    } catch (err) {
      console.error('Error parsing browser message:', err);
    }
  });
  
  ws.on('close', () => {
    console.log(`Browser disconnected: ${clientId}`);
    browserClients.delete(clientId);
  });
  
  ws.on('error', (err) => {
    console.error(`Browser error: ${clientId}`, err);
    browserClients.delete(clientId);
  });
});

// ============================================
// WebSocket Server - ESP32 Connections
// ============================================

wssESP32.on('connection', (ws, req) => {
  const esp32Id = Date.now().toString();
  const ip = req.socket.remoteAddress;
  
  console.log(`ESP32 connected: ${esp32Id} from ${ip}`);
  
  // Store ESP32 client
  esp32Clients.set(esp32Id, {
    id: esp32Id,
    socket: ws,
    address: ip,
    sticks: [],
    lastSeen: Date.now()
  });
  
  // Send identification
  ws.send(JSON.stringify({
    type: 'serverIdentify',
    serverId: 'fire-sticks-server',
    esp32Id: esp32Id
  }));
  
  // Handle messages from ESP32
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log(`ESP32 ${esp32Id} message:`, data);
      
      const client = esp32Clients.get(esp32Id);
      if (!client) return;
      
      // Update last seen
      client.lastSeen = Date.now();
      
      switch (data.type) {
        case 'identify':
          // ESP32 identifies itself
          client.info = data.info;
          client.sticks = data.sticks || [];
          break;
          
        case 'status':
          // ESP32 status update
          client.status = data.status;
          break;
          
        case 'stickConfig':
          // ESP32 reports its stick configuration
          client.sticks = data.sticks || [];
          break;
          
        default:
          // Forward to all browsers
          broadcastToBrowsers({
            type: 'esp32Message',
            esp32Id: esp32Id,
            data: data
          });
      }
      
      // Notify browsers about ESP32 update
      broadcastToBrowsers({
        type: 'esp32Update',
        esp32Id: esp32Id,
        sticks: client.sticks,
        info: client.info
      });
      
    } catch (err) {
      console.error('Error parsing ESP32 message:', err);
    }
  });
  
  ws.on('close', () => {
    console.log(`ESP32 disconnected: ${esp32Id}`);
    esp32Clients.delete(esp32Id);
    
    // Notify browsers
    broadcastToBrowsers({
      type: 'esp32Disconnected',
      esp32Id: esp32Id
    });
  });
  
  ws.on('error', (err) => {
    console.error(`ESP32 error: ${esp32Id}`, err);
    esp32Clients.delete(esp32Id);
  });
});

// ============================================
// Helper Functions
// ============================================

function broadcastToBrowsers(message) {
  const data = JSON.stringify(message);
  browserClients.forEach((ws) => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(data);
    }
  });
}

function broadcastToESP32s(message) {
  const data = JSON.stringify(message);
  esp32Clients.forEach((client) => {
    if (client.socket.readyState === WebSocket.OPEN) {
      client.socket.send(data);
    }
  });
}

function broadcastToESP32(esp32Id, message) {
  const client = esp32Clients.get(esp32Id);
  if (client && client.socket.readyState === WebSocket.OPEN) {
    client.socket.send(JSON.stringify(message));
  }
}

// ============================================
// Periodic Tasks
// ============================================

// Clean up disconnected clients
setInterval(() => {
  const now = Date.now();
  
  // Check browser clients
  for (const [id, ws] of browserClients) {
    if (ws.readyState !== WebSocket.OPEN) {
      browserClients.delete(id);
    }
  }
  
  // Check ESP32 clients
  for (const [id, client] of esp32Clients) {
    if (client.socket.readyState !== WebSocket.OPEN) {
      esp32Clients.delete(id);
    } else if (now - client.lastSeen > 30000) {
      // Timeout after 30 seconds of inactivity
      client.socket.close();
      esp32Clients.delete(id);
    }
  }
}, 10000);

// ============================================
// Start Server
// ============================================

server.listen(PORT, () => {
  console.log(`Fire Sticks Server running on port ${PORT}`);
  console.log(`Web interface: http://localhost:${PORT}`);
  console.log(`WebSocket (Browser): ws://localhost:${PORT}/ws/browser`);
  console.log(`WebSocket (ESP32): ws://localhost:${PORT}/ws/esp32`);
  console.log('');
  console.log('ESP32 Configuration:');
  console.log(`  - Connect ESP32s to: ws://<server-ip>:${PORT}/ws/esp32`);
  console.log(`  - Or use direct mode: ESP32 creates its own WiFi AP`);
});

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('Shutting down...');
  
  // Close all WebSocket connections
  browserClients.forEach((ws) => ws.close());
  esp32Clients.forEach((client) => client.socket.close());
  
  server.close(() => {
    process.exit(0);
  });
});

module.exports = { app, server, wssBrowser, wssESP32, broadcastToBrowsers, broadcastToESP32s };
