/// @file    FireSticks.ino
/// @brief   Fire simulation and control for LED strips
/// @example FireSticks.ino

#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// LED Configuration
#define DATA_PIN    5
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    30
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          48
#define FRAMES_PER_SECOND  60

// WiFi Configuration - Visible SSID
const char* ssid = "FireSticks-AP";
const char* password = "fire2026";
const bool ssid_hidden = false;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Fire Simulation Parameters
float cooling = 55;
float sparking = 120;
int heat[NUM_LEDS];

// Current mode and colors
String currentMode = "fire";
CRGB currentColor = CRGB::Red;
int colorIndex = 0;

// Sensor input (optional potentiometer)
int sensorPin = A0;
int sensorValue = 0;

// WebSocket message handling
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, R"({"type":"connected","leds":240,"mode":"fire"})");
      }
      break;
    case WStype_TEXT:
      {
        Serial.printf("[%u] Message: %s\n", num, payload);
        
        // Parse JSON message
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
          if (doc.containsKey("mode")) {
            currentMode = doc["mode"].as<String>();
            Serial.print("Mode changed to: ");
            Serial.println(currentMode);
          }
          if (doc.containsKey("color")) {
            int r = doc["color"][0];
            int g = doc["color"][1];
            int b = doc["color"][2];
            currentColor = CRGB(r, g, b);
            Serial.print("Color changed to: ");
            Serial.print(r);
            Serial.print(",");
            Serial.print(g);
            Serial.print(",");
            Serial.println(b);
          }
          if (doc.containsKey("cooling")) {
            cooling = doc["cooling"];
          }
          if (doc.containsKey("sparking")) {
            sparking = doc["sparking"];
          }
          if (doc.containsKey("brightness")) {
            FastLED.setBrightness(doc["brightness"]);
          }
          if (doc.containsKey("pixel")) {
            int idx = doc["pixel"]["index"];
            int r = doc["pixel"]["color"][0];
            int g = doc["pixel"]["color"][1];
            int b = doc["pixel"]["color"][2];
            if (idx >= 0 && idx < NUM_LEDS) {
              leds[idx] = CRGB(r, g, b);
            }
          }
          if (doc.containsKey("pattern")) {
            String pattern = doc["pattern"];
            applyPattern(pattern);
          }
        }
      }
      break;
  }
}

// Apply pattern to all LEDs
void applyPattern(String pattern) {
  if (pattern == "fire") {
    currentMode = "fire";
  } else if (pattern == "solid") {
    currentMode = "solid";
    fill_solid(leds, NUM_LEDS, currentColor);
  } else if (pattern == "rainbow") {
    currentMode = "rainbow";
    fill_rainbow(leds, NUM_LEDS, colorIndex);
    colorIndex++;
  } else if (pattern == "off") {
    currentMode = "off";
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}

// Fire simulation effect
void fireEffect() {
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < sparking ) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160,255) );
  }

  // Step 4.  Map from heat cells to LED colors
  // White at base (high heat), orange/yellow/red at tips (low heat)
  for( int j = 0; j < NUM_LEDS; j++) {
    uint8_t heatLevel = heat[j];
    
    // Map heat to color with white at highest intensity
    if (heatLevel > 220) {
      // White at peak heat
      leds[j] = CRGB::White;
    } else if (heatLevel > 180) {
      // White to orange transition
      uint8_t whiteMix = map(heatLevel, 180, 220, 255, 0);
      uint8_t orangeMix = map(heatLevel, 180, 220, 0, 255);
      leds[j] = CRGB(255, 165, 0).lerp8(CRGB::White, whiteMix);
    } else if (heatLevel > 140) {
      // Orange
      leds[j] = CRGB(255, 165, 0);
    } else if (heatLevel > 100) {
      // Orange to yellow transition
      uint8_t ratio = map(heatLevel, 100, 140, 0, 255);
      leds[j] = CRGB(255, 165, 0).lerp8(CRGB(255, 255, 0), ratio);
    } else if (heatLevel > 60) {
      // Yellow
      leds[j] = CRGB(255, 255, 0);
    } else if (heatLevel > 20) {
      // Yellow to red transition
      uint8_t ratio = map(heatLevel, 20, 60, 0, 255);
      leds[j] = CRGB(255, 255, 0).lerp8(CRGB::Red, ratio);
    } else {
      // Red at lowest heat, fading to black
      leds[j] = CRGB::Red;
      leds[j].fadeToBlackBy(map(heatLevel, 0, 20, 255, 0));
    }
  }
}

// Rainbow effect
void rainbowEffect() {
  fill_rainbow(leds, NUM_LEDS, colorIndex);
  colorIndex++;
  if (colorIndex > 255) colorIndex = 0;
}

// Setup
void setup() {
  delay(3000); // 3 second delay for recovery
  
  // Initialize serial first for debugging
  Serial.begin(115200);
  delay(1000); // Extra delay for ESP32-C3 serial initialization
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  // Set master brightness
  FastLED.setBrightness(BRIGHTNESS);
  
  // Initialize heat array for fire effect
  memset(heat, 0, sizeof(heat));

  // Start WiFi Access Point
  Serial.println("Starting WiFi Access Point...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("Hidden: ");
  Serial.println(ssid_hidden ? "Yes" : "No");
  
  bool apSuccess = WiFi.softAP(ssid, password, 6, ssid_hidden, 4);
  
  if (apSuccess) {
    Serial.println("Access Point started SUCCESSFULLY");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.print("Station MAC: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println("ERROR: Failed to start Access Point!");
  }
  
  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
  
  // Setup HTTP server for serving web interface
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", 
      "<html><head><title>Fire Sticks</title></head>"
      "<body><h1>Fire Sticks Control</h1>"
      "<p>Connect to WebSocket at ws://" + WiFi.softAPIP().toString() + ":81</p>"
      "<p><a href='/control'>Control Panel</a></p>"
      "</body></html>"
    );
  });
  
  server.on("/control", HTTP_GET, []() {
    server.send(200, "text/html", 
      "<html><head><title>Fire Sticks Control</title></head>"
      "<body><h1>LED Control</h1>"
      "<p>WebSocket: ws://" + WiFi.softAPIP().toString() + ":81</p>"
      "<div id='status'>Connecting...</div>"
      "<script>"
      "var socket = new WebSocket('ws://" + WiFi.softAPIP().toString() + ":81');"
      "socket.onopen = function(e) { document.getElementById('status').innerHTML = 'Connected'; };"
      "socket.onclose = function(e) { document.getElementById('status').innerHTML = 'Disconnected'; };"
      "function sendMode(mode) { socket.send(JSON.stringify({mode: mode})); }"
      "function sendColor(r, g, b) { socket.send(JSON.stringify({color: [r, g, b]})); }"
      "</script>"
      "</body></html>"
    );
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

// Main loop
void loop() {
  // Handle WebSocket and HTTP
  webSocket.loop();
  server.handleClient();
  
  // Read sensor if available
  sensorValue = analogRead(sensorPin);
  
  // Apply effects based on mode
  if (currentMode == "fire") {
    fireEffect();
  } else if (currentMode == "rainbow") {
    rainbowEffect();
  } else if (currentMode == "solid") {
    fill_solid(leds, NUM_LEDS, currentColor);
  }
  
  // Show LEDs
  FastLED.show();
  
  // Control frame rate
  FastLED.delay(1000/FRAMES_PER_SECOND);
}
