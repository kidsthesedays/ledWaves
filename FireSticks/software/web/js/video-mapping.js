// Fire Sticks - Video Mapping Interface
// Handles video input, draggable LED stick representations, and pixel sampling

class VideoMapper {
  constructor(p, videoElementId, canvasContainerId) {
    this.p = p;
    this.videoElement = document.getElementById(videoElementId);
    this.container = document.getElementById(canvasContainerId);
    
    // Video state
    this.video = null;
    this.videoLoaded = false;
    this.videoWidth = 640;
    this.videoHeight = 480;
    
    // LED sticks
    this.sticks = [];
    this.nextStickId = 1;
    
    // Sampling
    this.sampleCount = 240; // Default for 1m strip
    this.showSamplingPoints = true;
    
    // UI state
    this.draggingStick = null;
    this.draggingPoint = null;
    this.selectedStick = null;
    this.showGrid = false;
    
    // Initialize
    this.init();
  }
  
  init() {
    // Create video capture
    this.setupVideo();
    
    // Create canvas for mapping
    this.canvas = this.p.createCanvas(this.videoWidth, this.videoHeight);
    this.canvas.parent(this.container);
    this.canvas.id('video-mapping-canvas');
    this.canvas.style('border', '2px solid #ff6b35');
    this.canvas.style('border-radius', '8px');
    
    // Add event listeners
    this.canvas.elt.onmousedown = this.handleMouseDown.bind(this);
    this.canvas.elt.onmousemove = this.handleMouseMove.bind(this);
    this.canvas.elt.onmouseup = this.handleMouseUp.bind(this);
    this.canvas.elt.onmouseleave = this.handleMouseLeave.bind(this);
    
    // Touch support
    this.canvas.elt.ontouchstart = this.handleTouchStart.bind(this);
    this.canvas.elt.ontouchmove = this.handleTouchMove.bind(this);
    this.canvas.elt.ontouchend = this.handleTouchEnd.bind(this);
    
    // Add some default sticks
    this.addStick(50, 50, 200, 20);
    this.addStick(50, 100, 200, 20);
    this.addStick(50, 150, 200, 20);
    
    // Start update loop
    this.p.frameRate(30);
  }
  
  setupVideo() {
    // Create video element if it doesn't exist
    if (!this.videoElement) {
      this.videoElement = this.p.createVideo();
      this.videoElement.id('video-input');
      this.videoElement.style('display', 'none');
      document.body.appendChild(this.videoElement.elt);
    }
    
    this.video = this.videoElement;
    
    // Try to access camera
    this.startCamera();
    
    // Fallback to test pattern if camera fails
    setTimeout(() => {
      if (!this.videoLoaded) {
        this.createTestPattern();
      }
    }, 2000);
  }
  
  startCamera() {
    if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
      navigator.mediaDevices.getUserMedia({ video: true })
        .then(stream => {
          this.video.elt.srcObject = stream;
          this.video.elt.play();
          this.videoLoaded = true;
          
          // Resize canvas to match video
          this.video.elt.onloadedmetadata = () => {
            this.videoWidth = this.video.elt.videoWidth;
            this.videoHeight = this.video.elt.videoHeight;
            this.p.resizeCanvas(this.videoWidth, this.videoHeight);
          };
        })
        .catch(err => {
          console.error('Camera access denied:', err);
          this.createTestPattern();
        });
    } else {
      this.createTestPattern();
    }
  }
  
  createTestPattern() {
    // Create a test pattern video
    this.videoLoaded = true;
    
    // Set up a canvas to generate test pattern
    const testCanvas = document.createElement('canvas');
    testCanvas.width = this.videoWidth;
    testCanvas.height = this.videoHeight;
    const ctx = testCanvas.getContext('2d');
    
    // Generate a gradient test pattern
    const gradient = ctx.createLinearGradient(0, 0, this.videoWidth, this.videoHeight);
    gradient.addColorStop(0, '#ff0000');
    gradient.addColorStop(0.25, '#00ff00');
    gradient.addColorStop(0.5, '#0000ff');
    gradient.addColorStop(0.75, '#ffff00');
    gradient.addColorStop(1, '#ff00ff');
    
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, this.videoWidth, this.videoHeight);
    
    // Add grid
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.3)';
    ctx.lineWidth = 1;
    for (let i = 0; i < this.videoWidth; i += 20) {
      ctx.beginPath();
      ctx.moveTo(i, 0);
      ctx.lineTo(i, this.videoHeight);
      ctx.stroke();
    }
    for (let i = 0; i < this.videoHeight; i += 20) {
      ctx.beginPath();
      ctx.moveTo(0, i);
      ctx.lineTo(this.videoWidth, i);
      ctx.stroke();
    }
    
    // Create video from canvas
    this.video.elt.src = testCanvas.toDataURL('image/webp');
    this.video.elt.loop = true;
    this.video.elt.play();
  }
  
  loadVideoFile(file) {
    const url = URL.createObjectURL(file);
    this.video.elt.src = url;
    this.video.elt.loop = true;
    this.video.elt.play();
    this.videoLoaded = true;
    
    this.video.elt.onloadedmetadata = () => {
      this.videoWidth = this.video.elt.videoWidth;
      this.videoHeight = this.video.elt.videoHeight;
      this.p.resizeCanvas(this.videoWidth, this.videoHeight);
    };
  }
  
  addStick(x, y, length, ledsPerMeter) {
    const id = this.nextStickId++;
    const stick = {
      id: id,
      x: x,
      y: y,
      length: length,
      ledsPerMeter: ledsPerMeter,
      angle: 0,
      color: this.p.color(255, 100, 0),
      selected: false,
      samplingPoints: []
    };
    
    // Calculate sampling points
    this.updateSamplingPoints(stick);
    
    this.sticks.push(stick);
    return stick;
  }
  
  removeStick(id) {
    this.sticks = this.sticks.filter(stick => stick.id !== id);
    if (this.selectedStick && this.selectedStick.id === id) {
      this.selectedStick = null;
    }
  }
  
  updateSamplingPoints(stick) {
    stick.samplingPoints = [];
    const numLEDs = Math.round(stick.length * stick.ledsPerMeter / 1000);
    
    for (let i = 0; i < numLEDs; i++) {
      const t = i / (numLEDs - 1);
      const x = stick.x + Math.cos(stick.angle) * stick.length * t;
      const y = stick.y + Math.sin(stick.angle) * stick.length * t;
      stick.samplingPoints.push({ x, y, index: i });
    }
    
    return stick.samplingPoints;
  }
  
  samplePixelAt(x, y) {
    if (!this.videoLoaded) {
      // Return test color based on position
      const hue = (x / this.videoWidth * 360 + y / this.videoHeight * 100) % 360;
      return this.p.color(this.p.HSBtoRGB(hue / 360, 1, 1));
    }
    
    // Sample from video
    if (this.video.elt.readyState >= this.video.elt.HAVE_CURRENT_DATA) {
      // Create a temporary canvas to sample from video
      const tempCanvas = document.createElement('canvas');
      tempCanvas.width = this.videoWidth;
      tempCanvas.height = this.videoHeight;
      const ctx = tempCanvas.getContext('2d');
      
      // Draw current video frame
      ctx.drawImage(this.video.elt, 0, 0, this.videoWidth, this.videoHeight);
      
      // Get pixel data
      const pixelData = ctx.getImageData(x, y, 1, 1).data;
      return this.p.color(pixelData[0], pixelData[1], pixelData[2]);
    }
    
    // Fallback
    return this.p.color(128, 128, 128);
  }
  
  sampleStick(stick) {
    const colors = [];
    
    for (const point of stick.samplingPoints) {
      const color = this.samplePixelAt(point.x, point.y);
      colors.push(color);
    }
    
    return colors;
  }
  
  sampleAllSticks() {
    const allColors = [];
    
    for (const stick of this.sticks) {
      const stickColors = this.sampleStick(stick);
      allColors.push({
        stickId: stick.id,
        colors: stickColors
      });
    }
    
    return allColors;
  }
  
  // Mouse/Touch event handlers
  handleMouseDown(e) {
    const rect = this.canvas.elt.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    
    // Check if clicking on a stick
    for (const stick of this.sticks) {
      // Simple line intersection check
      const dx = stick.length * Math.cos(stick.angle);
      const dy = stick.length * Math.sin(stick.angle);
      
      // Check if point is near the line
      const dist = this.pointToLineDistance(x, y, stick.x, stick.y, stick.x + dx, stick.y + dy);
      
      if (dist < 20) {
        this.draggingStick = stick;
        this.selectedStick = stick;
        
        // Check if clicking near start or end
        const distToStart = Math.sqrt((x - stick.x) ** 2 + (y - stick.y) ** 2);
        const distToEnd = Math.sqrt((x - (stick.x + dx)) ** 2 + (y - (stick.y + dy)) ** 2);
        
        if (distToStart < 15) {
          this.draggingPoint = 'start';
        } else if (distToEnd < 15) {
          this.draggingPoint = 'end';
        } else {
          this.draggingPoint = 'middle';
        }
        
        this.canvas.elt.style.cursor = 'grabbing';
        e.preventDefault();
        return;
      }
    }
    
    // If not clicking on a stick, check if we should add a new one
    if (e.ctrlKey || e.metaKey) {
      this.addStick(x, y, 200, this.sampleCount);
      e.preventDefault();
    }
  }
  
  handleMouseMove(e) {
    const rect = this.canvas.elt.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    
    if (this.draggingStick) {
      if (this.draggingPoint === 'start') {
        // Move start point
        this.draggingStick.x = x;
        this.draggingStick.y = y;
      } else if (this.draggingPoint === 'end') {
        // Resize stick
        const dx = x - this.draggingStick.x;
        const dy = y - this.draggingStick.y;
        this.draggingStick.length = Math.sqrt(dx * dx + dy * dy);
        this.draggingStick.angle = Math.atan2(dy, dx);
      } else {
        // Move entire stick
        const dx = x - this.dragStartX;
        const dy = y - this.dragStartY;
        this.draggingStick.x += dx;
        this.draggingStick.y += dy;
        this.dragStartX = x;
        this.dragStartY = y;
      }
      
      this.updateSamplingPoints(this.draggingStick);
      e.preventDefault();
    }
  }
  
  handleMouseUp(e) {
    if (this.draggingStick) {
      this.draggingStick = null;
      this.draggingPoint = null;
      this.canvas.elt.style.cursor = '';
      e.preventDefault();
    }
  }
  
  handleMouseLeave(e) {
    this.canvas.elt.style.cursor = '';
  }
  
  handleTouchStart(e) {
    e.preventDefault();
    const touch = e.touches[0];
    const rect = this.canvas.elt.getBoundingClientRect();
    const x = touch.clientX - rect.left;
    const y = touch.clientY - rect.top;
    
    // Similar to mouse down
    for (const stick of this.sticks) {
      const dx = stick.length * Math.cos(stick.angle);
      const dy = stick.length * Math.sin(stick.angle);
      
      const dist = this.pointToLineDistance(x, y, stick.x, stick.y, stick.x + dx, stick.y + dy);
      
      if (dist < 30) {
        this.draggingStick = stick;
        this.selectedStick = stick;
        
        const distToStart = Math.sqrt((x - stick.x) ** 2 + (y - stick.y) ** 2);
        const distToEnd = Math.sqrt((x - (stick.x + dx)) ** 2 + (y - (stick.y + dy)) ** 2);
        
        if (distToStart < 25) {
          this.draggingPoint = 'start';
        } else if (distToEnd < 25) {
          this.draggingPoint = 'end';
        } else {
          this.draggingPoint = 'middle';
        }
        
        this.dragStartX = x;
        this.dragStartY = y;
        return;
      }
    }
    
    // Add new stick on long press
    this.touchStartTime = Date.now();
    this.touchStartX = x;
    this.touchStartY = y;
  }
  
  handleTouchMove(e) {
    e.preventDefault();
    const touch = e.touches[0];
    const rect = this.canvas.elt.getBoundingClientRect();
    const x = touch.clientX - rect.left;
    const y = touch.clientY - rect.top;
    
    if (this.draggingStick) {
      if (this.draggingPoint === 'start') {
        this.draggingStick.x = x;
        this.draggingStick.y = y;
      } else if (this.draggingPoint === 'end') {
        const dx = x - this.draggingStick.x;
        const dy = y - this.draggingStick.y;
        this.draggingStick.length = Math.sqrt(dx * dx + dy * dy);
        this.draggingStick.angle = Math.atan2(dy, dx);
      } else {
        const dx = x - this.dragStartX;
        const dy = y - this.dragStartY;
        this.draggingStick.x += dx;
        this.draggingStick.y += dy;
        this.dragStartX = x;
        this.dragStartY = y;
      }
      
      this.updateSamplingPoints(this.draggingStick);
    }
  }
  
  handleTouchEnd(e) {
    e.preventDefault();
    
    // Check for long press to add new stick
    if (!this.draggingStick && this.touchStartTime) {
      const duration = Date.now() - this.touchStartTime;
      if (duration > 500) {
        this.addStick(this.touchStartX, this.touchStartY, 200, this.sampleCount);
      }
    }
    
    this.draggingStick = null;
    this.draggingPoint = null;
    this.touchStartTime = null;
  }
  
  // Geometry helpers
  pointToLineDistance(px, py, x1, y1, x2, y2) {
    const dx = x2 - x1;
    const dy = y2 - y1;
    const length = Math.sqrt(dx * dx + dy * dy);
    
    if (length === 0) {
      return Math.sqrt((px - x1) ** 2 + (py - y1) ** 2);
    }
    
    const t = ((px - x1) * dx + (py - y1) * dy) / (length * length);
    const closestX = x1 + t * dx;
    const closestY = y1 + t * dy;
    
    return Math.sqrt((px - closestX) ** 2 + (py - closestY) ** 2);
  }
  
  // Drawing
  draw() {
    // Draw video
    if (this.videoLoaded) {
      this.p.image(this.video, 0, 0, this.videoWidth, this.videoHeight);
    } else {
      // Draw test pattern background
      this.p.background(50);
      for (let i = 0; i < this.videoWidth; i += 20) {
        const hue = (i / this.videoWidth * 360) % 360;
        const col = this.p.color(this.p.HSBtoRGB(hue / 360, 1, 1));
        this.p.stroke(col);
        this.p.line(i, 0, i, this.videoHeight);
      }
    }
    
    // Draw grid if enabled
    if (this.showGrid) {
      this.p.stroke(100, 100, 100, 100);
      this.p.strokeWeight(1);
      for (let i = 0; i < this.videoWidth; i += 20) {
        this.p.line(i, 0, i, this.videoHeight);
      }
      for (let i = 0; i < this.videoHeight; i += 20) {
        this.p.line(0, i, this.videoWidth, i);
      }
    }
    
    // Draw sticks
    for (const stick of this.sticks) {
      this.drawStick(stick);
    }
    
    // Draw sampling points if enabled
    if (this.showSamplingPoints && this.selectedStick) {
      this.drawSamplingPoints(this.selectedStick);
    }
  }
  
  drawStick(stick) {
    const dx = stick.length * Math.cos(stick.angle);
    const dy = stick.length * Math.sin(stick.angle);
    
    // Draw line
    this.p.stroke(stick.selected ? 255 : 200);
    this.p.strokeWeight(stick.selected ? 4 : 2);
    this.p.noFill();
    this.p.line(stick.x, stick.y, stick.x + dx, stick.y + dy);
    
    // Draw endpoints
    this.p.fill(stick.selected ? this.p.color(255, 0, 0) : this.p.color(200, 0, 0));
    this.p.noStroke();
    this.p.ellipse(stick.x, stick.y, 10, 10);
    this.p.ellipse(stick.x + dx, stick.y + dy, 10, 10);
    
    // Draw length indicator
    this.p.fill(255);
    this.p.textSize(12);
    this.p.textAlign(this.p.CENTER, this.p.CENTER);
    const midX = stick.x + dx / 2;
    const midY = stick.y + dy / 2 - 15;
    this.p.text(`${Math.round(stick.length)}px`, midX, midY);
    
    // Draw LED count
    const ledCount = Math.round(stick.length * stick.ledsPerMeter / 1000);
    this.p.text(`${ledCount} LEDs`, midX, midY + 15);
  }
  
  drawSamplingPoints(stick) {
    this.p.fill(255, 255, 0);
    this.p.noStroke();
    
    for (const point of stick.samplingPoints) {
      this.p.ellipse(point.x, point.y, 5, 5);
      
      // Draw index
      this.p.fill(255);
      this.p.textSize(8);
      this.p.textAlign(this.p.CENTER, this.p.CENTER);
      this.p.text(point.index, point.x, point.y - 10);
    }
  }
  
  // Utility functions
  getStickById(id) {
    return this.sticks.find(stick => stick.id === id);
  }
  
  setSampleCount(count) {
    this.sampleCount = count;
    for (const stick of this.sticks) {
      stick.ledsPerMeter = count;
      this.updateSamplingPoints(stick);
    }
  }
  
  toggleGrid() {
    this.showGrid = !this.showGrid;
  }
  
  toggleSamplingPoints() {
    this.showSamplingPoints = !this.showSamplingPoints;
  }
  
  clearAllSticks() {
    this.sticks = [];
    this.selectedStick = null;
  }
  
  // Save/Load configuration
  saveConfiguration() {
    const config = {
      sticks: this.sticks.map(stick => ({
        id: stick.id,
        x: stick.x,
        y: stick.y,
        length: stick.length,
        angle: stick.angle,
        ledsPerMeter: stick.ledsPerMeter
      })),
      sampleCount: this.sampleCount
    };
    
    return JSON.stringify(config);
  }
  
  loadConfiguration(configString) {
    try {
      const config = JSON.parse(configString);
      this.sticks = [];
      this.nextStickId = 1;
      
      if (config.sampleCount) {
        this.sampleCount = config.sampleCount;
      }
      
      if (config.sticks) {
        for (const stickData of config.sticks) {
          const stick = {
            id: stickData.id,
            x: stickData.x,
            y: stickData.y,
            length: stickData.length,
            angle: stickData.angle || 0,
            ledsPerMeter: stickData.ledsPerMeter || this.sampleCount,
            color: this.p.color(255, 100, 0),
            selected: false,
            samplingPoints: []
          };
          
          this.updateSamplingPoints(stick);
          this.sticks.push(stick);
          this.nextStickId = Math.max(this.nextStickId, stick.id + 1);
        }
      }
      
      return true;
    } catch (e) {
      console.error('Error loading configuration:', e);
      return false;
    }
  }
}
