/**
 * p5js-simple WebSocket Server
 * 
 * A simple WebSocket server for p5.js <-> ESP32 communication
 * This allows p5.js sketches to communicate with ESP32 devices
 * without direct hardware dependencies.
 * 
 * Usage:
 *   node server.js [port]
 *   Default port: 8080
 */

const WebSocket = require('ws');
const http = require('http');
const express = require('express');
const path = require('path');

// Configuration
const DEFAULT_PORT = 8080;
const DEFAULT_HOST = '0.0.0.0';

// Parse command line arguments
const args = process.argv.slice(2);
const PORT = args[0] ? parseInt(args[0]) : DEFAULT_PORT;
const HOST = args[1] || DEFAULT_HOST;

// Create Express app for serving files
const app = express();
const server = http.createServer(app);

// Store connected clients
const clients = new Map(); // clientId -> { WebSocket, type, info }
let clientCounter = 0;

// ============================================
// Express HTTP Server - Serve static files
// ============================================

app.use(express.static(path.join(__dirname, '../../sketches')));
app.use(express.json());

app.get('/', (req, res) => {
  res.json({
    server: 'p5js-simple WebSocket Server',
    version: '1.0',
    port: PORT,
    clients: clients.size,
    endpoints: {
      http: `http://${HOST}:${PORT}`,
      websocket: `ws://${HOST}:${PORT}`
    }
  });
});

app.get('/status', (req, res) => {
  res.json({
    clients: Array.from(clients.values()).map(c => ({
      id: c.id,
      type: c.type,
      address: c.socket._socket.remoteAddress,
      connectedAt: c.connectedAt,
      info: c.info
    }))
  });
});

// ============================================
// WebSocket Server
// ============================================

const wss = new WebSocket.Server({ server });

wss.on('connection', (socket, req) => {
  const clientId = `client-${clientCounter++}`;
  const ip = req.socket.remoteAddress;
  
  console.log(`[${clientId}] New connection from ${ip}`);
  
  // Store client
  clients.set(clientId, {
    id: clientId,
    socket: socket,
    type: 'unknown',
    address: ip,
    connectedAt: new Date().toISOString(),
    info: {}
  });
  
  // Send welcome message
  socket.send(JSON.stringify({
    type: 'connected',
    server: 'p5js-simple',
    version: '1.0',
    clientId: clientId,
    timestamp: Date.now()
  }));
  
  // Handle messages
  socket.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      const client = clients.get(clientId);
      
      if (!client) return;
      
      // Log incoming message
      console.log(`[${clientId}] ${JSON.stringify(data).substring(0, 100)}...`);
      
      // Handle message based on type
      switch (data.type) {
        case 'identify':
          // Client identifies itself
          client.type = data.client || 'unknown';
          client.info = data.info || {};
          console.log(`[${clientId}] Identified as: ${client.type}`);
          
          // Notify other clients
          broadcast({
            type: 'clientConnected',
            clientId: clientId,
            clientType: client.type,
            info: client.info
          }, clientId);
          break;
          
        case 'ping':
          // Respond to ping
          socket.send(JSON.stringify({
            type: 'pong',
            timestamp: Date.now(),
            from: clientId
          }));
          break;
          
        case 'pixels':
        case 'pixel':
        case 'mode':
        case 'color':
        case 'brightness':
        case 'fireParams':
        case 'rainbowParams':
          // Forward LED control messages to all clients
          // This allows p5.js to control multiple ESP32s
          broadcast(data, clientId);
          break;
          
        case 'listClients':
          // Send list of connected clients
          socket.send(JSON.stringify({
            type: 'clients',
            clients: Array.from(clients.values()).map(c => ({
              id: c.id,
              type: c.type,
              address: c.address,
              info: c.info
            }))
          }));
          break;
          
        case 'disconnect':
          // Request to disconnect
          socket.close();
          break;
          
        default:
          // Forward unknown message types to all clients
          // This allows custom message types
          broadcast(data, clientId);
      }
      
    } catch (err) {
      console.error(`[${clientId}] Error parsing message:`, err);
      socket.send(JSON.stringify({
        type: 'error',
        code: 400,
        message: 'Invalid JSON message',
        timestamp: Date.now()
      }));
    }
  });
  
  // Handle close
  socket.on('close', () => {
    const client = clients.get(clientId);
    if (client) {
      console.log(`[${clientId}] Disconnected (${client.type})`);
      clients.delete(clientId);
      
      // Notify other clients
      broadcast({
        type: 'clientDisconnected',
        clientId: clientId
      });
    }
  });
  
  // Handle errors
  socket.on('error', (err) => {
    console.error(`[${clientId}] Error:`, err);
    clients.delete(clientId);
  });
});

// ============================================
// Helper Functions
// ============================================

/**
 * Broadcast message to all connected clients except sender
 */
function broadcast(message, excludeId = null) {
  const data = JSON.stringify(message);
  clients.forEach((client, id) => {
    if (id !== excludeId && client.socket.readyState === WebSocket.OPEN) {
      client.socket.send(data);
    }
  });
}

/**
 * Broadcast message to all clients
 */
function broadcastAll(message) {
  const data = JSON.stringify(message);
  clients.forEach((client, id) => {
    if (client.socket.readyState === WebSocket.OPEN) {
      client.socket.send(data);
    }
  });
}

// ============================================
// Start Server
// ============================================

server.listen(PORT, HOST, () => {
  console.log('========================================');
  console.log('  p5js-simple WebSocket Server');
  console.log('  Version: 1.0');
  console.log(`  HTTP: http://${HOST}:${PORT}`);
  console.log(`  WebSocket: ws://${HOST}:${PORT}`);
  console.log('========================================');
  console.log('');
  console.log('Available endpoints:');
  console.log(`  http://${HOST}:${PORT}/          - Server info`);
  console.log(`  http://${HOST}:${PORT}/status     - Client list`);
  console.log(`  ws://${HOST}:${PORT}/           - WebSocket`);
  console.log('');
  console.log('Press Ctrl+C to stop');
  console.log('');
});

// Handle process termination
process.on('SIGINT', () => {
  console.log('\nShutting down server...');
  
  // Notify all clients
  broadcastAll({
    type: 'serverShutdown',
    timestamp: Date.now()
  });
  
  server.close(() => {
    console.log('Server stopped');
    process.exit(0);
  });
  
  // Force exit after 2 seconds
  setTimeout(() => {
    console.log('Force exit');
    process.exit(1);
  }, 2000);
});
