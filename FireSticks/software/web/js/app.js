// Fire Sticks - LED Control Application
// Using p5.js for visualization and p5.gui for controls

// Configuration
const CONFIG = {
  numLEDs: 240,
  numSticks: 1,
  ledsPerStick: 240,
  defaultHost: '192.168.4.1',  // Default ESP32 AP IP
  defaultPort: 81,
  colors: {
    fire: [255, 100, 0],
    water: [0, 100, 255],
    forest: [0, 255, 0],
    electric: [100, 0, 255],
    pink: [255, 0, 255]
  }
};

// State
let state = {
  socket: null,
  isConnected: false,
  currentMode: 'fire',
  currentColor: [255, 100, 0],
  brightness: 96,
  cooling: 55,
  sparking: 120,
  leds: [],
  previewCanvas: null
};

// p5.js sketch for preview
let previewSketch = function(p) {
  let ledColors = [];
  let gui;
  
  // Parameters for GUI
  let params = {
    mode: 'fire',
    color: [255, 100, 0],
    brightness: 96,
    cooling: 55,
    sparking: 120,
    speed: 50,
    scale: 100,
    pattern: 'fire',
    numLEDs: CONFIG.numLEDs
  };
  
  p.setup = function() {
    // Create canvas for preview
    let container = document.getElementById('preview-canvas');
    p.canvas = p.createCanvas(container.offsetWidth, 300);
    p.canvas.parent('preview-canvas');
    
    // Initialize LED colors
    for (let i = 0; i < CONFIG.numLEDs; i++) {
      ledColors.push(p.color(0, 0, 0));
    }
    
    // Create GUI
    gui = p.createGui('LED Settings');
    gui.addObject(params);
    
    // Watch for changes
    p.createGuiWatchers();
    
    // Start fire simulation
    p.frameRate(30);
  };
  
  p.createGuiWatchers = function() {
    // This will be called when GUI values change
    // We'll use the draw loop to update based on params
  };
  
  p.draw = function() {
    // Clear canvas
    p.background(30, 30, 40);
    
    // Draw LED strip preview
    drawLEDStrip(p, ledColors);
    
    // Update LED colors based on mode
    updateLEDPreview(p, ledColors);
    
    // Send updates to ESP32 if connected
    if (state.isConnected && state.socket.readyState === WebSocket.OPEN) {
      // Send mode and color updates
      if (params.mode !== state.currentMode) {
        state.currentMode = params.mode;
        sendMessage({ mode: params.mode });
      }
      
      if (JSON.stringify(params.color) !== JSON.stringify(state.currentColor)) {
        state.currentColor = params.color.slice();
        sendMessage({ color: params.color });
      }
      
      if (params.brightness !== state.brightness) {
        state.brightness = params.brightness;
        sendMessage({ brightness: params.brightness });
      }
      
      if (params.cooling !== state.cooling) {
        state.cooling = params.cooling;
        sendMessage({ cooling: params.cooling });
      }
      
      if (params.sparking !== state.sparking) {
        state.sparking = params.sparking;
        sendMessage({ sparking: params.sparking });
      }
    }
  };
  
  p.windowResized = function() {
    let container = document.getElementById('preview-canvas');
    p.resizeCanvas(container.offsetWidth, 300);
  };
  
  // Make functions available globally
  p.drawLEDStrip = drawLEDStrip;
  p.updateLEDPreview = updateLEDPreview;
};

// Draw LED strip preview
function drawLEDStrip(p, colors) {
  const stripWidth = p.width - 40;
  const stripHeight = 20;
  const startX = 20;
  const startY = p.height / 2;
  
  // Draw strip background
  p.noStroke();
  p.fill(50, 50, 60);
  p.rect(startX, startY - stripHeight/2, stripWidth, stripHeight, 5);
  
  // Draw individual LEDs
  const ledWidth = stripWidth / CONFIG.numLEDs;
  for (let i = 0; i < CONFIG.numLEDs; i++) {
    const x = startX + i * ledWidth;
    const y = startY - stripHeight/2 + 2;
    
    p.noStroke();
    p.fill(colors[i]);
    p.rect(x, y, ledWidth - 1, stripHeight - 4, 2);
  }
  
  // Draw connection indicators
  p.fill(100, 100, 120);
  p.ellipse(startX, startY, 10, 10);
  p.ellipse(startX + stripWidth, startY, 10, 10);
  
  // Draw label
  p.fill(200, 200, 200);
  p.textSize(12);
  p.textAlign(p.LEFT, p.CENTER);
  p.text('Data In', startX - 5, startY);
  p.textAlign(p.RIGHT, p.CENTER);
  p.text('Data Out', startX + stripWidth + 5, startY);
}

// Update LED preview based on mode
function updateLEDPreview(p, colors) {
  const params = previewSketch.p.instances[0].params;
  
  switch(params.mode) {
    case 'fire':
      updateFirePreview(p, colors, params);
      break;
    case 'rainbow':
      updateRainbowPreview(p, colors, params);
      break;
    case 'solid':
      updateSolidPreview(p, colors, params);
      break;
    case 'off':
      updateOffPreview(p, colors);
      break;
  }
}

// Fire effect preview
function updateFirePreview(p, colors, params) {
  const now = p.millis();
  
  for (let i = 0; i < CONFIG.numLEDs; i++) {
    // Simulate fire effect
    const intensity = p.noise(i * 0.1, now * 0.001) * 255;
    const hue = p.map(intensity, 0, 255, 0, 30);
    const saturation = p.map(intensity, 0, 255, 50, 100);
    const value = p.map(intensity, 0, 255, 0, 100);
    
    colors[i] = p.color(p.HSBtoRGB(hue / 360, saturation / 100, value / 100));
  }
}

// Rainbow effect preview
function updateRainbowPreview(p, colors, params) {
  const now = p.millis();
  const hueOffset = p.map(now % 10000, 0, 10000, 0, 360);
  
  for (let i = 0; i < CONFIG.numLEDs; i++) {
    const hue = (i * (360 / CONFIG.numLEDs) + hueOffset) % 360;
    colors[i] = p.color(p.HSBtoRGB(hue / 360, 1, 1));
  }
}

// Solid color preview
function updateSolidPreview(p, colors, params) {
  for (let i = 0; i < CONFIG.numLEDs; i++) {
    colors[i] = p.color(params.color[0], params.color[1], params.color[2]);
  }
}

// Off preview
function updateOffPreview(p, colors) {
  for (let i = 0; i < CONFIG.numLEDs; i++) {
    colors[i] = p.color(0, 0, 0);
  }
}

// HSB to RGB conversion helper
p5.prototype.HSBtoRGB = function(h, s, v) {
  let r, g, b, i, f, p, q, t;
  
  h = h * 360;
  s = s * 100;
  v = v * 100;
  
  if (s === 0) {
    r = g = b = v;
  } else {
    h /= 60;
    i = Math.floor(h);
    f = h - i;
    p = v * (1 - s / 100);
    q = v * (1 - f * s / 100);
    t = v * (1 - (1 - f) * s / 100);
    
    switch (i) {
      case 0: r = v; g = t; b = p; break;
      case 1: r = q; g = v; b = p; break;
      case 2: r = p; g = v; b = t; break;
      case 3: r = p; g = q; b = v; break;
      case 4: r = t; g = p; b = v; break;
      default: r = v; g = p; b = q; break;
    }
  }
  
  return [r / 100 * 255, g / 100 * 255, b / 100 * 255];
};

// WebSocket functions
function connectWebSocket(host = CONFIG.defaultHost, port = CONFIG.defaultPort) {
  const url = `ws://${host}:${port}`;
  
  console.log(`Connecting to WebSocket at ${url}`);
  updateStatus('Connecting...', false);
  
  state.socket = new WebSocket(url);
  
  state.socket.onopen = function(e) {
    console.log('WebSocket connected');
    state.isConnected = true;
    updateStatus('Connected', true);
    
    // Send initial state
    sendMessage({
      type: 'init',
      mode: state.currentMode,
      color: state.currentColor,
      brightness: state.brightness
    });
  };
  
  state.socket.onclose = function(e) {
    console.log('WebSocket disconnected');
    state.isConnected = false;
    updateStatus('Disconnected', false);
    
    // Attempt to reconnect after delay
    setTimeout(() => connectWebSocket(host, port), 5000);
  };
  
  state.socket.onerror = function(e) {
    console.error('WebSocket error:', e);
    state.isConnected = false;
    updateStatus('Connection Error', false);
  };
  
  state.socket.onmessage = function(e) {
    const message = JSON.parse(e.data);
    console.log('Received:', message);
    
    if (message.type === 'connected') {
      console.log(`ESP32 has ${message.leds} LEDs, mode: ${message.mode}`);
      CONFIG.numLEDs = message.leds || CONFIG.numLEDs;
    }
  };
}

// Send message to ESP32
function sendMessage(message) {
  if (state.isConnected && state.socket.readyState === WebSocket.OPEN) {
    const jsonMessage = JSON.stringify(message);
    console.log('Sending:', jsonMessage);
    state.socket.send(jsonMessage);
  } else {
    console.log('Not connected, message not sent:', message);
  }
}

// Update connection status display
function updateStatus(text, isConnected) {
  document.getElementById('status-text').textContent = text;
  const indicator = document.getElementById('status-indicator');
  
  if (isConnected) {
    indicator.classList.add('connected');
    document.body.classList.add('connected');
  } else {
    indicator.classList.remove('connected');
    document.body.classList.remove('connected');
  }
}

// Set mode functions
function setMode(mode) {
  state.currentMode = mode;
  if (previewSketch.p && previewSketch.p.instances[0]) {
    previewSketch.p.instances[0].params.mode = mode;
  }
  sendMessage({ mode: mode });
}

// Set pixel color
function setPixel() {
  const index = parseInt(document.getElementById('pixel-index').value);
  const colorHex = document.getElementById('pixel-color').value;
  
  // Convert hex to RGB
  const r = parseInt(colorHex.substring(1, 3), 16);
  const g = parseInt(colorHex.substring(3, 5), 16);
  const b = parseInt(colorHex.substring(5, 7), 16);
  
  sendMessage({
    pixel: {
      index: index,
      color: [r, g, b]
    }
  });
  
  // Update preview
  if (previewSketch.p && previewSketch.p.instances[0]) {
    const colors = previewSketch.p.instances[0].ledColors;
    if (index >= 0 && index < colors.length) {
      colors[index] = previewSketch.p.instances[0].color(r, g, b);
    }
  }
}

// Connect button function
function connectToESP32() {
  const host = prompt('Enter ESP32 IP address:', CONFIG.defaultHost);
  if (host) {
    CONFIG.defaultHost = host;
    connectWebSocket(host);
  }
}

// Initialize application
function initApp() {
  // Create p5.js instance for preview
  new p5(previewSketch, 'preview-canvas');
  
  // Connect to WebSocket
  connectWebSocket();
  
  // Add global functions
  window.setMode = setMode;
  window.setPixel = setPixel;
  window.connectToESP32 = connectToESP32;
  
  // Add keyboard shortcuts
  document.addEventListener('keydown', function(e) {
    // F1: Toggle connection
    if (e.key === 'F1') {
      e.preventDefault();
      if (state.isConnected) {
        state.socket.close();
      } else {
        connectWebSocket();
      }
    }
    
    // F2-F5: Mode shortcuts
    if (e.key === 'F2') { e.preventDefault(); setMode('fire'); }
    if (e.key === 'F3') { e.preventDefault(); setMode('rainbow'); }
    if (e.key === 'F4') { e.preventDefault(); setMode('solid'); }
    if (e.key === 'F5') { e.preventDefault(); setMode('off'); }
  });
  
  console.log('Fire Sticks application initialized');
}

// Start application when DOM is loaded
document.addEventListener('DOMContentLoaded', initApp);
