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
#define NUM_LEDS    240
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  60

// WiFi Configuration
const char* ssid = "FireSticks-AP";
const char* password = "firesicks2024";

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
        webSocket.sendTXT(num, "{"type":"connected","leds":240,"mode":"fire"}");
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
            currentMode = doc["mode"];
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
  for( int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor(heat[j]);
    int pixelHue = ((j / NUM_LEDS) * 255) + (millis() / 10);
    color = color.lerp(CRGB::White, 32);
    leds[j] = color;
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
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  // Set master brightness
  FastLED.setBrightness(BRIGHTNESS);

  Serial.begin(115200);
  
  // Initialize heat array for fire effect
  memset(heat, 0, sizeof(heat));

  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  
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
