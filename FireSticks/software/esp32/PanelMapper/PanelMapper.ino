/// @file    PanelMapper.ino
/// @brief   Zigzag LED panel mapper — a single strip driven as an X/Y grid,
///          with live-adjustable geometry and a phone-friendly web UI.
///
/// Coordinates are standard graphics orientation: (0,0) is the top-left
/// corner, X grows right, Y grows down. Flip toggles in the UI re-map the
/// logical origin onto physically rotated/mirrored panels.
/// Connect to the access point "FireSticks-Panel" (password fire2026),
/// then open http://192.168.4.1

#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Physical strip (compile-time)
#define DATA_PIN     5
#define LED_TYPE     WS2811
#define COLOR_ORDER  GRB
#define NUM_LEDS     300
#define FRAMES_PER_SECOND 60

// Grid limits (live-adjustable at runtime)
#define MAX_COLS     64
#define MAX_ROWS     64

// WiFi
const char* ssid = "FireSticks-Panel";
const char* password = "fire2026";

WebServer server(80);
WebSocketsServer webSocket(81);
Preferences prefs;

CRGB leds[NUM_LEDS];

// ---- Live panel geometry ----
// One continuous strip, snaking through the grid from the top-left corner.
// Row y starts at startPad plus the accumulated length of every earlier row
// plus one turnGap at each U-turn point. Rows can either all share `cols`
// pixels or have individual lengths (`rowLens`); flipX/flipY re-map the
// logical top-left origin onto the physical panel orientation.
// `startPad` pixels at the very beginning of the strip and `endPad` at the
// end are also kept unlit.
uint16_t cols    = 16;   // default pixels per row (uniform mode)
uint16_t rows    = 16;   // number of rows
uint16_t turnGap = 1;    // dead pixels at each U-turn point
uint16_t startPad = 0;   // unlit pixels at strip start
uint16_t endPad   = 0;   // unlit pixels at strip end
bool     uniformRows = true;
bool     flipX    = false;
bool     flipY    = false;

uint16_t rowLens[MAX_ROWS];         // per-row pixel counts (non-uniform mode)
uint32_t rowBase[MAX_ROWS + 1];     // first physical LED of each row
uint32_t cellPrefix[MAX_ROWS + 1];  // cumulative grid-cell count before each row
uint16_t maxRowLen = cols;

uint8_t brightness = 48;
uint8_t speed      = 50;
String  mode       = "glow";
CRGB    baseColor  = CRGB(255, 96, 0);

// Per-cell paint overrides (tap a cell in the web preview)
CRGB painted[MAX_COLS * MAX_ROWS];
bool paintedOn[MAX_COLS * MAX_ROWS];

// ---------------------------------------------------------------- mapping

uint16_t rowLenOf(uint16_t y) {
  if (y >= rows) return 0;
  return uniformRows ? cols : rowLens[y];
}

// Recompute row start offsets and the used-LED count after any geometry change
void recomputeGeometry() {
  rowBase[0] = startPad;
  cellPrefix[0] = 0;
  maxRowLen = 1;
  for (uint16_t y = 0; y < rows; y++) {
    uint16_t len = rowLenOf(y);
    if (len > maxRowLen) maxRowLen = len;
    rowBase[y + 1] = rowBase[y] + len + turnGap;
    cellPrefix[y + 1] = cellPrefix[y] + len;
  }
}

// Physical pixels consumed by the current layout (no trailing gap after the
// final row — that dead run belongs to endPad territory)
uint32_t usedLeds() {
  if (rows == 0) return startPad + endPad;
  return rowBase[rows - 1] + rowLenOf(rows - 1);
}

// X/Y grid position (logical, origin top-left) -> physical strip index,
// or -1 if that cell has no LED.
int16_t xyToIndex(uint16_t x, uint16_t y) {
  if (y >= rows) return -1;
  uint16_t len = rowLenOf(y);
  if (x >= len) return -1;
  uint16_t yc = flipY ? (rows - 1 - y) : y;
  uint16_t xc = flipX ? (len - 1 - x) : x;
  uint32_t idx = rowBase[yc];
  // Even physical rows run left->right, odd rows run right->left
  idx += (yc & 1) ? (len - 1 - xc) : xc;
  if (idx >= NUM_LEDS || idx >= (uint32_t)(NUM_LEDS - endPad)) return -1;
  return (int16_t)idx;
}

// ---------------------------------------------------------------- effects

CRGB pixelColor(uint16_t x, uint16_t y) {
  uint16_t maxDim = (maxRowLen > rows) ? maxRowLen : rows;
  uint16_t cx = maxRowLen / 2, cy = rows / 2;
  uint16_t dx = (x > cx) ? x - cx : cx - x;
  uint16_t dy = (y > cy) ? y - cy : cy - y;
  uint8_t d = sqrt16(dx * dx + dy * dy);

  if (mode == "solid") {
    return baseColor;

  } else if (mode == "glow") {
    uint8_t breath = beatsin8(speed + 5, 40, 255);
    uint16_t fall = 255 - ((uint32_t)d * 480 / maxDim);
    uint8_t v = scale8(breath, (uint8_t)max((int)5, (int)fall));
    CRGB c = baseColor;
    c.nscale8_video(v);
    return c;

  } else if (mode == "ripple") {
    uint16_t spacing = 512 / maxDim;
    uint8_t t = (millis() * speed) / 8;
    uint8_t v = sin8(d * spacing - t);
    CRGB c = baseColor;
    c.nscale8_video(v);
    return c;

  } else if (mode == "rainbow") {
    uint8_t t = millis() / (220 - speed * 2);
    return CHSV(x * 255 / maxRowLen + y * 20 + t, 255, 255);

  } else if (mode == "wavex") {
    uint16_t spacing = 512 / maxRowLen;
    uint8_t t = (millis() * speed) / 8;
    uint8_t v = sin8(x * spacing - t);
    CRGB c = baseColor;
    c.nscale8_video(v);
    return c;

  } else if (mode == "wavey") {
    uint16_t spacing = 512 / maxDim;
    uint8_t t = (millis() * speed) / 8;
    uint8_t v = sin8(y * spacing - t);
    CRGB c = baseColor;
    c.nscale8_video(v);
    return c;

  } else if (mode == "fire") {
    uint8_t t = (millis() * speed) / 32;
    uint8_t n = inoise8(x * 90, y * 90 - t * 6, t);
    return ColorFromPalette(HeatColors_p, qadd8(n, 40));

  } else if (mode == "test") {
    uint16_t n = cellPrefix[rows];
    uint16_t i = (millis() / (uint16_t)(310 - speed * 3)) % n;
    if (i == cellPrefix[y] + x) return CRGB::White;
    return CRGB(2, 2, 2);
  }

  return CRGB::Black;
}

void render() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  if (mode == "off") return;

  for (uint16_t y = 0; y < rows; y++) {
    for (uint16_t x = 0; x < cols; x++) {
      int16_t idx = xyToIndex(x, y);
      if (idx < 0) continue;
      CRGB c = pixelColor(x, y);
      uint32_t cell = (uint32_t)y * MAX_COLS + x;
      if (cell < (uint32_t)MAX_COLS * MAX_ROWS && paintedOn[cell]) c = painted[cell];
      leds[idx] = c;
    }
  }

  // In test mode, mark the dead/unlit regions so wiring can be verified
  if (mode == "test") {
    for (uint16_t i = 0; i < startPad && i < NUM_LEDS; i++) leds[i] = CRGB(0, 0, 60);
    for (uint16_t i = 0; i < endPad; i++) {
      uint32_t tail = NUM_LEDS - 1 - i;
      if (tail < NUM_LEDS) leds[tail] = CRGB(0, 0, 60);
    }
    for (uint16_t y = 0; y < rows; y++) {
      for (uint16_t k = 0; k < turnGap; k++) {
        uint32_t g = rowBase[y] + rowLenOf(y) + k;
        if (g < NUM_LEDS) leds[g] = CRGB(40, 0, 0);
      }
    }
  }
}

// ---------------------------------------------------------------- config I/O

void sendConfig(uint8_t client = 255) {
  StaticJsonDocument<1024> doc;
  doc["type"] = "config";
  doc["cols"] = cols;
  doc["rows"] = rows;
  doc["gap"] = turnGap;
  doc["startPad"] = startPad;
  doc["endPad"] = endPad;
  doc["uniform"] = uniformRows;
  doc["flipX"] = flipX;
  doc["flipY"] = flipY;
  JsonArray lens = doc.createNestedArray("rowLens");
  for (uint16_t y = 0; y < rows && y < MAX_ROWS; y++) lens.add(rowLens[y]);
  doc["brightness"] = brightness;
  doc["speed"] = speed;
  doc["mode"] = mode;
  JsonArray col = doc.createNestedArray("color");
  col.add(baseColor.r); col.add(baseColor.g); col.add(baseColor.b);
  doc["used"] = usedLeds();
  doc["total"] = NUM_LEDS;
  String out;
  serializeJson(doc, out);
  if (client == 255) webSocket.broadcastTXT(out);
  else webSocket.sendTXT(client, out);
}

void sendFrame() {
  String out = "{\"type\":\"frame\",\"w\":" + String(maxRowLen) + ",\"h\":" + String(rows) + ",\"l\":[";
  for (uint16_t y = 0; y < rows; y++) {
    if (y) out += ',';
    out += String(rowLenOf(y));
  }
  out += "],\"d\":[";
  out.reserve(6 * cellPrefix[rows] + 64);
  bool first = true;
  for (uint16_t y = 0; y < rows; y++) {
    for (uint16_t x = 0; x < rowLenOf(y); x++) {
      int16_t idx = xyToIndex(x, y);
      CRGB c = 0;
      if (idx >= 0) c = leds[idx];
      if (!first) out += ',';
      char hex[9];
      snprintf(hex, sizeof(hex), "\"%02x%02x%02x\"", c.r, c.g, c.b);
      out += hex;
      first = false;
    }
  }
  out += "]}";
  webSocket.broadcastTXT(out);
}

void clearPaint() {
  memset(paintedOn, 0, sizeof(paintedOn));
}

void saveSettings() {
  prefs.putUShort("cols", cols);
  prefs.putUShort("rows", rows);
  prefs.putUShort("gap", turnGap);
  prefs.putUShort("startPad", startPad);
  prefs.putUShort("endPad", endPad);
  prefs.putBool("uniform", uniformRows);
  prefs.putBool("flipX", flipX);
  prefs.putBool("flipY", flipY);
  prefs.putBytes("rowLens", (const uint8_t*)rowLens, sizeof(rowLens));
  prefs.putUChar("brightness", brightness);
  prefs.putUChar("speed", speed);
  prefs.putString("mode", mode);
  prefs.putUChar("cr", baseColor.r);
  prefs.putUChar("cg", baseColor.g);
  prefs.putUChar("cb", baseColor.b);
  Serial.println("Settings saved to flash");
}

void loadSettings() {
  if (!prefs.begin("panelmap", true)) return;
  cols     = prefs.getUShort("cols", cols);
  rows     = prefs.getUShort("rows", rows);
  turnGap  = prefs.getUShort("gap", turnGap);
  startPad = prefs.getUShort("startPad", startPad);
  endPad   = prefs.getUShort("endPad", endPad);
  uniformRows = prefs.getBool("uniform", uniformRows);
  flipX    = prefs.getBool("flipX", flipX);
  flipY    = prefs.getBool("flipY", flipY);
  size_t lensBytes = prefs.getBytesLength("rowLens");
  if (lensBytes == sizeof(rowLens)) {
    prefs.getBytes("rowLens", (void*)rowLens, sizeof(rowLens));
  } else {
    for (uint16_t y = 0; y < MAX_ROWS; y++) rowLens[y] = cols;
  }
  brightness = prefs.getUChar("brightness", brightness);
  speed    = prefs.getUChar("speed", speed);
  mode     = prefs.getString("mode", mode);
  baseColor = CRGB(prefs.getUChar("cr", 255), prefs.getUChar("cg", 96), prefs.getUChar("cb", 0));
  prefs.end();
}

void applyField(const char* key, JsonVariant v) {
  uint32_t val = v.as<uint32_t>();
  if (!strcmp(key, "cols") && val >= 1 && val <= MAX_COLS) {
    cols = val;
    if (uniformRows) {
      for (uint16_t y = 0; y < MAX_ROWS; y++) rowLens[y] = cols;
    }
  }
  else if (!strcmp(key, "rows") && val >= 1 && val <= MAX_ROWS) rows = val;
  else if (!strcmp(key, "gap") && val <= 40) turnGap = val;
  else if (!strcmp(key, "startPad") && val <= NUM_LEDS) startPad = val;
  else if (!strcmp(key, "endPad") && val <= NUM_LEDS) endPad = val;
  else if (!strcmp(key, "uniform")) {
    for (uint16_t y = 0; y < MAX_ROWS; y++) rowLens[y] = cols;
    uniformRows = (val != 0);
  }
  else if (!strcmp(key, "flipX")) flipX = (val != 0);
  else if (!strcmp(key, "flipY")) flipY = (val != 0);
  else if (!strcmp(key, "brightness")) { brightness = val; FastLED.setBrightness(brightness); }
  else if (!strcmp(key, "speed") && val >= 1 && val <= 100) speed = val;
}

// ---------------------------------------------------------------- web

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("[%u] Client connected\n", num);
    sendConfig(num);
    return;
  }
  if (type != WStype_TEXT) return;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;

  const char* keys[] = {"cols", "rows", "gap", "startPad", "endPad", "brightness", "speed", "uniform", "flipX", "flipY"};
  for (uint8_t i = 0; i < 10; i++) {
    if (doc.containsKey(keys[i])) applyField(keys[i], doc[keys[i]]);
  }
  if (doc.containsKey("rowLens")) {
    uniformRows = false;
    uint16_t y = 0;
    for (JsonVariant len : doc["rowLens"].as<JsonArray>()) {
      if (y >= rows || y >= MAX_ROWS) break;
      uint32_t v = len.as<uint32_t>();
      rowLens[y] = (v >= 1 && v <= MAX_COLS) ? v : 1;
      y++;
    }
  }
  if (doc.containsKey("mode")) mode = doc["mode"].as<String>();
  if (doc.containsKey("color")) {
    baseColor = CRGB(doc["color"][0] | 0, doc["color"][1] | 0, doc["color"][2] | 0);
  }
  if (doc.containsKey("pixel")) {
    uint16_t px = doc["pixel"]["x"] | 0, py = doc["pixel"]["y"] | 0;
    uint32_t cell = (uint32_t)py * MAX_COLS + px;
    if (px < rowLenOf(py) && (uint32_t)py < MAX_ROWS && cell < (uint32_t)MAX_COLS * MAX_ROWS) {
      painted[cell] = CRGB(doc["pixel"]["color"][0] | 0, doc["pixel"]["color"][1] | 0, doc["pixel"]["color"][2] | 0);
      paintedOn[cell] = true;
    }
  }
  if (doc.containsKey("clearPaint")) clearPaint();
  if (doc.containsKey("save")) saveSettings();

  recomputeGeometry();
  sendConfig(num);
}

const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>FireSticks Panel Mapper</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: -apple-system, system-ui, sans-serif; background: #111; color: #eee; padding: 12px; }
  h1 { font-size: 1.2rem; }
  .sub { font-size: .75rem; color: #888; margin-top: 2px; }
  .status { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: #c33; margin-right: 6px; }
  .status.on { background: #3c3; }
  .card { background: #1c1c1e; border-radius: 12px; padding: 12px; margin-top: 12px; }
  h2 { font-size: .85rem; text-transform: uppercase; color: #999; margin-bottom: 10px; letter-spacing: .05em; }
  canvas { width: 100%; touch-action: manipulation; display: block; }
  .row { display: flex; align-items: center; gap: 10px; margin: 10px 0; }
  .row label { width: 42%; font-size: .85rem; }
  .row output { width: 3.2em; text-align: right; font-variant-numeric: tabular-nums; }
  input[type=range] { flex: 1; }
  input[type=color] { width: 100%; height: 44px; border: none; border-radius: 8px; background: none; }
  .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }
  .grid.three-2 { grid-template-columns: repeat(2, 1fr); }
  button { padding: 12px 6px; border: none; border-radius: 8px; background: #2c2c2e; color: #eee; font-size: .85rem; }
  button.active { background: #e80; color: #fff; font-weight: 600; }
  button.save { background: #262; }
  #info { font-size: .75rem; color: #888; margin-top: 8px; }
  #info.bad { color: #f66; }
  .chk { display: flex; align-items: center; gap: 8px; font-size: .85rem; margin: 10px 0; }
  .chk input { width: 18px; height: 18px; }
  #rowInputs { display: grid; grid-template-columns: repeat(8, 1fr); gap: 6px; }
  #rowInputs input { width: 100%; padding: 8px 2px; border: none; border-radius: 6px; background: #2c2c2e; color: #eee; text-align: center; }
  #rowInputs input:focus { outline: 2px solid #e80; }
</style>
</head>
<body>
<h1><span class="status" id="dot"></span>FireSticks Panel Mapper</h1>
<div class="sub">Zigzag grid from a single strip &mdash; live geometry tuning</div>

<div class="card">
  <h2>Panel Preview (tap to paint)</h2>
  <canvas id="cv" width="320" height="320"></canvas>
  <div id="info"></div>
</div>

<div class="card">
  <h2>Geometry</h2>
  <label class="chk"><input type="checkbox" id="uniform" checked> Same length for every row</label>
  <div class="row" id="colsRow"><label>Columns / row</label><input type="range" id="cols" min="1" max="64" value="16"><output id="o-cols">16</output></div>
  <div class="row"><label>Rows</label><input type="range" id="rows" min="1" max="64" value="16"><output id="o-rows">16</output></div>
  <div id="rowEditor" style="display:none">
    <h2>Row lengths (px each)</h2>
    <div id="rowInputs"></div>
  </div>
  <div class="row"><label>U-turn dead px</label><input type="range" id="gap" min="0" max="40" value="1"><output id="o-gap">1</output></div>
  <div class="row"><label>Strip start pad</label><input type="range" id="startPad" min="0" max="100" value="0"><output id="o-startPad">0</output></div>
  <div class="row"><label>Strip end pad</label><input type="range" id="endPad" min="0" max="100" value="0"><output id="o-endPad">0</output></div>
  <div class="row" style="gap:24px">
    <label class="chk" style="margin:0"><input type="checkbox" id="flipX"> Flip X</label>
    <label class="chk" style="margin:0"><input type="checkbox" id="flipY"> Flip Y</label>
  </div>
  <div class="grid three-2" style="margin-top:8px">
    <button class="save" id="saveBtn">Save to flash</button>
    <button id="clearBtn">Clear paint</button>
  </div>
</div>

<div class="card">
  <h2>Color</h2>
  <input type="color" id="color" value="#ff6000">
  <div class="row" style="margin-top:8px"><label>Brightness</label><input type="range" id="brightness" min="0" max="255" value="48"><output id="o-brightness">48</output></div>
  <div class="row"><label>Speed</label><input type="range" id="speed" min="1" max="100" value="50"><output id="o-speed">50</output></div>
</div>

<div class="card">
  <h2>Effects</h2>
  <div class="grid">
    <button data-mode="glow">Glow</button>
    <button data-mode="ripple">Ripple</button>
    <button data-mode="wavex">Wave X</button>
    <button data-mode="wavey">Wave Y</button>
    <button data-mode="fire">Fire</button>
    <button data-mode="rainbow">Rainbow</button>
    <button data-mode="solid">Solid</button>
    <button data-mode="test">Test run</button>
    <button data-mode="off">Off</button>
  </div>
</div>

<script>
var ws, cfg = {cols:16,rows:16,gap:1,startPad:0,endPad:0,uniform:true,flipX:false,flipY:false,brightness:48,speed:50,mode:'glow',used:0,total:300};
var lengths = [], frame = null, pushTimer = null, lensTimer = null, lastRowKey = null;
var cv = document.getElementById('cv'), ctx = cv.getContext('2d');

function rebuildLengths() {
  if (cfg.uniform) {
    lengths = [];
    for (var y = 0; y < cfg.rows; y++) lengths.push(cfg.cols);
  } else {
    lengths = [];
    for (var y = 0; y < cfg.rows; y++) lengths.push(cfg.rowLens[y] || cfg.cols);
  }
}

function connect() {
  ws = new WebSocket('ws://' + location.host + ':81');
  ws.onopen = function() { document.getElementById('dot').classList.add('on'); };
  ws.onclose = function() { document.getElementById('dot').classList.remove('on'); setTimeout(connect, 1500); };
  ws.onmessage = function(e) {
    var m = JSON.parse(e.data);
    if (m.type === 'config') {
      var geoChanged = (m.cols !== cfg.cols || m.rows !== cfg.rows || m.gap !== cfg.gap ||
                        m.startPad !== cfg.startPad || m.endPad !== cfg.endPad ||
                        m.uniform !== cfg.uniform || '' + m.rowLens !== '' + cfg.rowLens ||
                        m.flipX !== cfg.flipX || m.flipY !== cfg.flipY);
      cfg = m; rebuildLengths(); syncUI();
      if (geoChanged) { frame = null; resizeCanvas(); }
    } else if (m.type === 'frame') { frame = m; }
  };
}

function send(obj) { if (ws && ws.readyState === 1) ws.send(JSON.stringify(obj)); }

function syncUI() {
  ['cols','rows','gap','startPad','endPad','brightness','speed'].forEach(function(k) {
    var el = document.getElementById(k);
    if (+el.value !== cfg[k]) el.value = cfg[k];
    document.getElementById('o-' + k).textContent = cfg[k];
  });
  document.getElementById('uniform').checked = !!cfg.uniform;
  document.getElementById('flipX').checked = !!cfg.flipX;
  document.getElementById('flipY').checked = !!cfg.flipY;
  document.getElementById('colsRow').style.display = cfg.uniform ? '' : 'none';
  document.getElementById('rowEditor').style.display = cfg.uniform ? 'none' : '';
  rebuildRowInputs();
  var used = cfg.startPad + lengths.reduce(function(a, b) { return a + b; }, 0) +
             Math.max(0, cfg.rows - 1) * cfg.gap + cfg.endPad;
  var info = document.getElementById('info');
  info.textContent = 'Layout uses ' + used + ' of ' + cfg.total + ' LEDs';
  info.className = used > cfg.total ? 'bad' : '';
  document.querySelectorAll('[data-mode]').forEach(function(b) {
    b.classList.toggle('active', b.dataset.mode === cfg.mode);
  });
}

function rebuildRowInputs() {
  var key = cfg.rows + '|' + cfg.uniform + '|' + lengths.join(',');
  if (key === lastRowKey) return;
  lastRowKey = key;
  var box = document.getElementById('rowInputs');
  box.innerHTML = '';
  lengths.forEach(function(len, y) {
    var inp = document.createElement('input');
    inp.type = 'number'; inp.min = 1; inp.max = 64; inp.value = len;
    inp.addEventListener('input', function() {
      lengths[y] = Math.min(64, Math.max(1, +this.value || 1));
      clearTimeout(lensTimer);
      lensTimer = setTimeout(function() { send({rowLens: lengths.slice()}); }, 250);
    });
    box.appendChild(inp);
  });
}

function maxLen() { return Math.max.apply(null, lengths.concat([1])); }

function resizeCanvas() {
  var px = cv.clientWidth || 320;
  var cw = Math.max(4, Math.floor(px / Math.max(maxLen(), 8)));
  cv.width = maxLen() * cw;
  cv.height = cfg.rows * cw;
}

function draw() {
  resizeCanvas();
  var cw = cv.width / maxLen(), ch = cv.height / cfg.rows;
  var offs = [0];
  for (var i = 0; i < lengths.length; i++) offs.push(offs[i] + lengths[i]);
  ctx.fillStyle = '#000'; ctx.fillRect(0, 0, cv.width, cv.height);
  ctx.strokeStyle = '#333'; ctx.fillStyle = '#777'; ctx.lineWidth = 1;
  for (var y = 0; y < cfg.rows; y++) {
    var left = !(y & 1);
    for (var x = 0; x < lengths[y]; x++) {
      var c = '#101018';
      if (frame && frame.d[offs[y] + x]) c = '#' + frame.d[offs[y] + x];
      ctx.fillStyle = c;
      ctx.fillRect(x * cw + 1, y * ch + 1, cw - 2, ch - 2);
    }
    ctx.beginPath();
    ctx.moveTo(0, y * ch); ctx.lineTo(lengths[y] * cw, y * ch);
    ctx.stroke();
    ctx.font = Math.max(9, ch * 0.45) + 'px sans-serif';
    ctx.fillText(left ? '1\u2192' : '\u21901', left ? 3 : lengths[y] * cw - 14, (y + 0.7) * ch);
  }
  requestAnimationFrame(draw);
}

cv.addEventListener('pointerdown', function(e) {
  var r = cv.getBoundingClientRect();
  var x = Math.floor((e.clientX - r.left) / r.width * maxLen());
  var y = Math.floor((e.clientY - r.top) / r.height * cfg.rows);
  if (x < 0 || y < 0 || y >= cfg.rows) return;
  if (x >= (lengths[y] || 0)) return;
  var hex = document.getElementById('color').value;
  send({pixel: {x: x, y: y, color: [parseInt(hex.substr(1,2),16), parseInt(hex.substr(3,2),16), parseInt(hex.substr(5,2),16)]}});
});

['cols','rows','gap','startPad','endPad','brightness','speed'].forEach(function(k) {
  document.getElementById(k).addEventListener('input', function() {
    document.getElementById('o-' + k).textContent = this.value;
    if (k === 'cols' || k === 'rows') {
      frame = null;
      var v = +this.value;
      if (k === 'rows') {
        var nl = lengths.slice(0, v);
        while (nl.length < v) nl.push(+document.getElementById('cols').value);
        lengths = nl;
        cfg.rows = v;
      } else {
        if (cfg.uniform) lengths = lengths.map(function() { return v; });
        cfg.cols = v;
      }
      rebuildRowInputs();
    }
    clearTimeout(pushTimer);
    var v = +this.value;
    pushTimer = setTimeout(function() {
      var o = {}; o[k] = v; send(o);
      if (k === 'cols' || k === 'rows') resizeCanvas();
    }, 120);
  });
});

document.getElementById('uniform').addEventListener('change', function() {
  send({uniform: this.checked ? 1 : 0});
  var v = +document.getElementById('cols').value;
  lengths = lengths.map(function() { return v; });
  frame = null; rebuildRowInputs(); resizeCanvas();
});

document.getElementById('flipX').addEventListener('change', function() { send({flipX: this.checked ? 1 : 0}); });
document.getElementById('flipY').addEventListener('change', function() { send({flipY: this.checked ? 1 : 0}); });

document.getElementById('color').addEventListener('input', function() {
  var hex = this.value;
  send({color: [parseInt(hex.substr(1,2),16), parseInt(hex.substr(3,2),16), parseInt(hex.substr(5,2),16)]});
});

document.querySelectorAll('[data-mode]').forEach(function(b) {
  b.addEventListener('click', function() { send({mode: b.dataset.mode}); });
});

document.getElementById('saveBtn').addEventListener('click', function() { send({save: true}); });
document.getElementById('clearBtn').addEventListener('click', function() { send({clearPaint: true}); });

connect(); syncUI(); draw();
</script>
</body>
</html>
)HTML";

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== FireSticks Panel Mapper ===");

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  clearPaint();

  loadSettings();
  for (uint16_t y = 0; y < MAX_ROWS; y++) if (rowLens[y] < 1 || rowLens[y] > MAX_COLS) rowLens[y] = cols;
  recomputeGeometry();
  FastLED.setBrightness(brightness);
  Serial.printf("Grid %ux%u, gap %u, pad %u/%u, %u/%u LEDs used\n",
                cols, rows, turnGap, startPad, endPad, (unsigned)usedLeds(), NUM_LEDS);

  WiFi.softAP(ssid, password, 6, false, 4);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("HTTP + WebSocket servers started");
}

void loop() {
  webSocket.loop();
  server.handleClient();
  render();

  EVERY_N_MILLISECONDS(120) { sendFrame(); }

  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}
