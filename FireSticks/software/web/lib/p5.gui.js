// p5.gui - Modified version for Fire Sticks project
// Original by Bitcraft Lab - https://github.com/bitcraftlab/p5.gui
// Modified for Fire Sticks LED control system

(function() {

  // list of guis
  var guis = [];

  // default slider params
  var sliderMin = 0;
  var sliderMax = 100;
  var sliderStep = 1;

  // default gui provider
  var guiProvider = 'QuickSettings';

  const defaultLabel = 'p5.gui';

  // Create a GUI using QuickSettings (or DAT.GUI or ...)
  // You only need to pass a reference to the sketch in instance mode

  // Usually you will call createGui(this, 'label');
  p5.prototype.createGui = function(sketch, label, provider) {

    // createGui(label) signature
    if ((typeof sketch) === 'string') {
      return this.createGui(label, sketch, provider);
    }

    // normally the sketch will just be embedded below the body
    let parent = document.body;

    if(sketch === undefined) {
      // p5js global mode
      sketch = window;
      label = label || document.title || defaultLabel;
    } else {
      // p5js instance mode
      parent = sketch.canvas.parentElement;
      label = label || parent.id || defaultLabel;
    }

    if(!('color' in sketch)) {
      console.error(`${parent.id}: You need to pass the p5 sketch to createGui in instance mode!`);
    }

    // default gui provider
    provider = provider || guiProvider;

    var gui;

    // create a gui using the provider
    if(provider === 'QuickSettings') {
      if(QuickSettings) {
        console.log('Creating p5.gui powered by QuickSettings.');
        gui = new QSGui(label, parent, sketch);
      } else {
        console.log('QuickSettings not found. Is the script included in your HTML?');
        gui = new DummyGui(label, parent, sketch);
      }
    } else {
      console.log('Unknown GUI provider ' + provider);
      gui = new DummyGui(label, parent, sketch);
    }

    // add it to the list of guis
    guis.push(gui);

    // return it
    return gui;

  };


  p5.prototype.removeGui = function(gui) {
    // TODO: implement this
  };

  // update defaults used for creation of sliders
  p5.prototype.sliderRange = function(vmin, vmax, vstep) {
    sliderMin = vmin;
    sliderMax = vmax;
    sliderStep = vstep;
  };

  // extend default behaviour of noLoop()
  p5.prototype.noLoop = function() {
    this._loop = false;
    for(var i = 0; i < guis.length; i++) {
      guis[i].noLoop();
    }
  };

  // extend default behaviour of loop()
  p5.prototype.loop = function() {
    for(var i = 0; i < guis.length; i++) {
      guis[i].loop();
    }
    this._loop = true;
    this._draw();
  };

  // QuickSettings GUI implementation
  var QSGui = function(label, parent, sketch) {
    this.parent = parent;
    this.sketch = sketch;
    this.label = label;
    
    // Create container
    this.container = document.createElement('div');
    this.container.className = 'quicksettings';
    parent.appendChild(this.container);
    
    // Create QuickSettings instance
    this.qs = new QuickSettings(label, this.container);
    
    // Store added globals for updating
    this.globals = [];
    this.objects = [];
    
    // Create a placeholder for the GUI
    this.placeHolder = document.createElement('div');
    this.placeHolder.className = 'p5gui-placeholder';
    this.container.appendChild(this.placeHolder);
  };

  QSGui.prototype.addGlobals = function() {
    for(var i = 0; i < arguments.length; i++) {
      this.globals.push(arguments[i]);
      this.addGlobal(arguments[i]);
    }
  };

  QSGui.prototype.addGlobal = function(name) {
    var obj = {};
    obj[name] = this.sketch[name];
    this.addObject(obj);
  };

  QSGui.prototype.addObject = function(obj) {
    this.objects.push(obj);
    
    // Build QuickSettings controls
    for(var key in obj) {
      if(obj.hasOwnProperty(key)) {
        this.addControl(key, obj[key]);
      }
    }
  };

  QSGui.prototype.addControl = function(name, value) {
    var self = this;
    
    if(value instanceof Array) {
      // Check if it's a color array
      if(value.length >= 3 && typeof value[0] === 'number') {
        this.qs.addColor(name, value);
      } else {
        this.qs.addDropdown(name, value);
      }
    } else if(typeof value === 'boolean') {
      this.qs.addBoolean(name, value);
    } else if(typeof value === 'number') {
      // Check for magic variables
      var minVal = this.sketch[`${name}Min`] !== undefined ? this.sketch[`${name}Min`] : sliderMin;
      var maxVal = this.sketch[`${name}Max`] !== undefined ? this.sketch[`${name}Max`] : sliderMax;
      var stepVal = this.sketch[`${name}Step`] !== undefined ? this.sketch[`${name}Step`] : sliderStep;
      
      this.qs.addRange(name, minVal, maxVal, value, stepVal);
    } else if(typeof value === 'string') {
      this.qs.addText(name, value);
    }
    
    // Add change listener
    this.qs.bindRange(name, function(val) {
      self.sketch[name] = val;
      // Update in all objects that have this property
      for(var i = 0; i < self.objects.length; i++) {
        if(self.objects[i].hasOwnProperty(name)) {
          self.objects[i][name] = val;
        }
      }
    });
    
    this.qs.bindBoolean(name, function(val) {
      self.sketch[name] = val;
      for(var i = 0; i < self.objects.length; i++) {
        if(self.objects[i].hasOwnProperty(name)) {
          self.objects[i][name] = val;
        }
      }
    });
    
    this.qs.bindColor(name, function(val) {
      self.sketch[name] = val;
      for(var i = 0; i < self.objects.length; i++) {
        if(self.objects[i].hasOwnProperty(name)) {
          self.objects[i][name] = val;
        }
      }
    });
    
    this.qs.bindDropdown(name, function(val) {
      self.sketch[name] = val;
      for(var i = 0; i < self.objects.length; i++) {
        if(self.objects[i].hasOwnProperty(name)) {
          self.objects[i][name] = val;
        }
      }
    });
    
    this.qs.bindText(name, function(val) {
      self.sketch[name] = val;
      for(var i = 0; i < self.objects.length; i++) {
        if(self.objects[i].hasOwnProperty(name)) {
          self.objects[i][name] = val;
        }
      }
    });
  };

  QSGui.prototype.noLoop = function() {
    // QuickSettings doesn't have a noLoop method
  };

  QSGui.prototype.loop = function() {
    // QuickSettings doesn't have a loop method
  };

  QSGui.prototype.show = function() {
    this.container.style.display = 'block';
  };

  QSGui.prototype.hide = function() {
    this.container.style.display = 'none';
  };

  QSGui.prototype.moveTo = function(x, y) {
    this.container.style.position = 'absolute';
    this.container.style.left = x + 'px';
    this.container.style.top = y + 'px';
  };

  // Dummy GUI implementation (fallback)
  var DummyGui = function(label, parent, sketch) {
    this.parent = parent;
    this.sketch = sketch;
    this.label = label;
    
    this.container = document.createElement('div');
    this.container.className = 'p5gui-dummy';
    this.container.innerHTML = '<h3>' + label + '</h3><p>GUI library not loaded</p>';
    parent.appendChild(this.container);
  };

  DummyGui.prototype.addGlobals = function() {};
  DummyGui.prototype.addObject = function() {};
  DummyGui.prototype.noLoop = function() {};
  DummyGui.prototype.loop = function() {};
  DummyGui.prototype.show = function() {};
  DummyGui.prototype.hide = function() {};
  DummyGui.prototype.moveTo = function() {};

})();
