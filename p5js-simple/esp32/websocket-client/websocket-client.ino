/// @file    websocket-client.ino
/// @brief   Minimal WebSocket client for ESP32
///          Receives pixel data and commands from p5.js via WebSocket
/// @example websocket-client.ino
///
/// This firmware connects to a WebSocket server and listens for
/// LED control commands from p5.js sketches.
///
/// Dependencies:
///   - WiFi (built-in)
///   - WebSockets by Markus Sattler
///   - FastLED
///   - ArduinoJson

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <FastLED.h>
#include <ArduinoJson.h>

// ============================================
// LED Configuration
// ============================================

#define DATA_PIN    5           // GPIO pin for LED data
#define LED_TYPE    WS2812B    // WS2811, WS2812, WS2812B
#define COLOR_ORDER GRB         // GRB, RGB, BRG, etc.
#define NUM_LEDS    240        // Number of LEDs in strip

CRGB leds[NUM_LEDS];

#define BRIGHTNESS  96         // Default brightness (0-255)
#define FRAME_RATE   60         // Target frame rate

// ============================================
// WiFi Configuration
// ============================================

// Option 1: Connect to existing WiFi network
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Option 2: Create Access Point (uncomment to use)
// const char* ssid = "ESP32-LEDs";
// const char* password = "ledcontrol";
// bool useAP = true;

bool useAP = false;  // Set to true to use Access Point mode

// ============================================
// WebSocket Configuration
// ============================================

const char* websocket_host = "192.168.1.100";  // WebSocket server IP
const int websocket_port = 8080;               // WebSocket server port
const char* websocket_path = "/";              // WebSocket path

WebSocketsClient webSocket;

// ============================================
// State Variables
// ============================================

String currentMode = "off";
CRGB currentColor = CRGB::Black;
int currentBrightness = BRIGHTNESS;

// Fire simulation variables
float cooling = 55;
float sparking = 120;
int heat[NUM_LEDS];

// ============================================
// Setup
// ============================================

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for serial to initialize
  
  Serial.println("\n=== ESP32 WebSocket Client ===");
  Serial.println("Compiled: " __DATE__ " " __TIME__);
  
  // Initialize LED strip
  Serial.println("\nInitializing LED strip...");
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  Serial.println("LED strip initialized");
  
  // Initialize heat array for fire effect
  memset(heat, 0, sizeof(heat));
  
  // Connect to WiFi
  connectWiFi();
  
  // Connect to WebSocket server
  connectWebSocket();
  
  Serial.println("\nSetup complete!");
}

// ============================================
// WiFi Connection
// ============================================

void connectWiFi() {
  Serial.println("\nConnecting to WiFi...");
  
  if (useAP) {
    // Create Access Point
    Serial.println("Creating Access Point...");
    Serial.print("SSID: ");
    Serial.println(ssid);
    
    bool apSuccess = WiFi.softAP(ssid, password);
    if (apSuccess) {
      Serial.println("Access Point created!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.softAPIP());
    } else {
      Serial.println("ERROR: Failed to create Access Point!");
    }
  } else {
    // Connect to existing network
    Serial.print("SSID: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("MAC Address: ");
      Serial.println(WiFi.macAddress());
    } else {
      Serial.println("\nERROR: Failed to connect to WiFi!");
      Serial.println("Trying Access Point mode...");
      useAP = true;
      connectWiFi();
    }
  }
}

// ============================================
// WebSocket Connection
// ============================================

void connectWebSocket() {
  Serial.println("\nConnecting to WebSocket server...");
  Serial.print("Host: ");
  Serial.println(websocket_host);
  Serial.print("Port: ");
  Serial.println(websocket_port);
  
  webSocket.begin(websocket_host, websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);  // Reconnect every 5 seconds
  
  Serial.println("WebSocket client started");
}

// ============================================
// WebSocket Event Handler
// ============================================

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Disconnected");
      break;
      
    case WStype_CONNECTED:
      {
        Serial.println("[WebSocket] Connected!");
        
        // Send identification
        String identifyMsg = R"({"type":"identify","client":"esp32","name":"WebSocket Client","leds":)" + String(NUM_LEDS) + R"(","firmware":"websocket-client","version":"1.0"})";
        webSocket.sendTXT(identifyMsg);
        
        // Send initial status
        sendStatus();
      }
      break;
      
    case WStype_TEXT:
      {
        // Parse JSON message
        Serial.print("[WebSocket] Message: ");
        Serial.println((char*)payload);
        
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
          Serial.print("JSON parse error: ");
          Serial.println(error.c_str());
          return;
        }
        
        // Handle message
        handleMessage(doc);
      }
      break;
      
    case WStype_BIN:
      Serial.println("[WebSocket] Binary data received");
      break;
      
    case WStype_ERROR:
      Serial.println("[WebSocket] Error");
      break;
      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      Serial.println("[WebSocket] Fragmented message");
      break;
      
    case WStype_PING:
      Serial.println("[WebSocket] Ping received");
      webSocket.sendTXT(R"({"type":"pong"})");
      break;
      
    case WStype_PONG:
      Serial.println("[WebSocket] Pong received");
      break;
  }
}

// ============================================
// Message Handler
// ============================================

void handleMessage(StaticJsonDocument<1024> &doc) {
  if (!doc.containsKey("type")) {
    Serial.println("Message has no type field");
    return;
  }
  
  String type = doc["type"];
  
  // Mode commands
  if (type == "mode") {
    if (doc.containsKey("mode")) {
      currentMode = doc["mode"];
      Serial.print("Mode set to: ");
      Serial.println(currentMode);
    }
  }
  
  // Color command
  else if (type == "color") {
    if (doc.containsKey("color") && doc["color"].is<JsonArray>()) {
      JsonArray colorArray = doc["color"];
      if (colorArray.size() >= 3) {
        currentColor = CRGB(
          colorArray[0] | 0,
          colorArray[1] | 0,
          colorArray[2] | 0
        );
        Serial.print("Color set to: ");
        Serial.print(colorArray[0]);
        Serial.print(",");
        Serial.print(colorArray[1]);
        Serial.print(",");
        Serial.println(colorArray[2]);
      }
    }
  }
  
  // Brightness command
  else if (type == "brightness") {
    if (doc.containsKey("value")) {
      currentBrightness = doc["value"] | 0;
      FastLED.setBrightness(currentBrightness);
      Serial.print("Brightness set to: ");
      Serial.println(currentBrightness);
    }
  }
  
  // Pixel data command
  else if (type == "pixels") {
    if (doc.containsKey("data") && doc.containsKey("count")) {
      JsonArray data = doc["data"];
      int count = doc["count"] | 0;
      
      int pixelsToUpdate = min(count, NUM_LEDS);
      for (int i = 0; i < pixelsToUpdate; i++) {
        if (data[i * 3].is<int>() && data[i * 3 + 1].is<int>() && data[i * 3 + 2].is<int>()) {
          leds[i] = CRGB(
            data[i * 3] | 0,
            data[i * 3 + 1] | 0,
            data[i * 3 + 2] | 0
          );
        }
      }
      FastLED.show();
      Serial.print("Updated ");
      Serial.print(pixelsToUpdate);
      Serial.println(" pixels");
    }
  }
  
  // Single pixel command
  else if (type == "pixel") {
    if (doc.containsKey("index") && doc.containsKey("color")) {
      int index = doc["index"] | 0;
      JsonArray colorArray = doc["color"];
      
      if (index >= 0 && index < NUM_LEDS && colorArray.size() >= 3) {
        leds[index] = CRGB(
          colorArray[0] | 0,
          colorArray[1] | 0,
          colorArray[2] | 0
        );
        FastLED.show();
        Serial.print("Set pixel ");
        Serial.print(index);
        Serial.println(" to new color");
      }
    }
  }
  
  // Fire parameters
  else if (type == "fireParams") {
    if (doc.containsKey("cooling")) {
      cooling = doc["cooling"];
    }
    if (doc.containsKey("sparking")) {
      sparking = doc["sparking"];
    }
    Serial.print("Fire params: cooling=");
    Serial.print(cooling);
    Serial.print(", sparking=");
    Serial.println(sparking);
  }
  
  // Rainbow parameters
  else if (type == "rainbowParams") {
    // Handle rainbow parameters if needed
  }
  
  // Status request
  else if (type == "status") {
    sendStatus();
  }
  
  // Ping
  else if (type == "ping") {
    webSocket.sendTXT(R"({"type":"pong"})");
  }
  
  // Unknown message type
  else {
    Serial.print("Unknown message type: ");
    Serial.println(type);
  }
}

// ============================================
// Send Status
// ============================================

void sendStatus() {
  String statusMsg = R"({"type":"status","leds":)" + String(NUM_LEDS) + 
                   R"(","mode":")" + currentMode + 
                   R"(","brightness":)" + String(currentBrightness) + 
                   R"(","freeMemory":)" + String(ESP.getFreeHeap()) + 
                   R"(","uptime":)" + String(millis() / 1000) + 
                   R"("})" ;
  webSocket.sendTXT(statusMsg);
}

// ============================================
// LED Effects
// ============================================

// Fire effect
void fireEffect() {
  // Step 1: Cool down every cell
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }
  
  // Step 2: Heat drifts up and diffuses
  for (int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  
  // Step 3: Randomly ignite new sparks
  if (random8() < sparking) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }
  
  // Step 4: Map heat to LED colors
  for (int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor(heat[j]);
    int pixelHue = color.getHue();
    pixelHue = qsub8(pixelHue, 1);
    leds[j] = color;
  }
  
  FastLED.show();
}

// Rainbow effect
void rainbowEffect() {
  static uint8_t hue = 0;
  fill_rainbow(leds, NUM_LEDS, hue++);
  FastLED.show();
}

// Solid color effect
void solidEffect() {
  fill_solid(leds, NUM_LEDS, currentColor);
  FastLED.show();
}

// Off effect
void offEffect() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// ============================================
// Main Loop
// ============================================

void loop() {
  // Handle WebSocket events
  webSocket.loop();
  
  // Handle WiFi reconnection if needed
  if (!useAP && WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > 10000) {
      lastReconnectAttempt = now;
      Serial.println("WiFi disconnected, attempting to reconnect...");
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }
  
  // Run LED effects based on mode
  switch(currentMode.c_str()[0]) {
    case 'f':  // fire
      fireEffect();
      break;
    case 'r':  // rainbow
      rainbowEffect();
      break;
    case 's':  // solid
      solidEffect();
      break;
    case 'o':  // off
      offEffect();
      break;
    default:
      // Unknown mode, show current color
      fill_solid(leds, NUM_LEDS, currentColor);
      FastLED.show();
      break;
  }
  
  // Small delay to maintain frame rate
  delay(1000 / FRAME_RATE);
}

// ============================================
// Utility Functions
// ============================================

// Print WiFi status
void printWiFiStatus() {
  Serial.println("\nWiFi Status:");
  Serial.print("  SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("  IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("  Signal Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}
