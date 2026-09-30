# Communication Protocols

This document describes the communication protocols used between p5.js sketches, bridge servers, and microcontrollers.

## Table of Contents

1. [WebSocket Protocol](#websocket-protocol)
2. [Serial JSON Protocol](#serial-json-protocol)
3. [I2C Protocol](#i2c-protocol)
4. [Message Types](#message-types)
5. [Error Handling](#error-handling)

---

## WebSocket Protocol

### Overview

- **Transport**: WebSocket (ws:// or wss://)
- **Port**: 81 (ESP32 default) or 8080 (bridge server default)
- **Format**: JSON messages
- **Encoding**: UTF-8

### Connection Flow

```
1. Client (p5.js) connects to: ws://host:port/
2. Server sends: {"type": "connected", "id": "server-123", "protocol": "1.0"}
3. Client responds: {"type": "identify", "client": "p5js", "version": "1.0"}
4. Data exchange begins...
```

### Message Format

All messages are JSON objects with at least a `type` field:

```json
{
  "type": "messageType",
  "timestamp": 1234567890,
  "data": { ... }
}
```

### WebSocket Endpoints

| Path | Purpose | Direction |
|------|---------|-----------|
| `/` | Main data channel | Bidirectional |
| `/ws/browser` | Browser connections | Browser → Server |
| `/ws/esp32` | ESP32 connections | ESP32 → Server |
| `/ws/bridge` | Bridge connections | Bridge → Server |

---

## Serial JSON Protocol

### Overview

- **Transport**: Serial (UART)
- **Baud Rate**: 115200
- **Format**: JSON messages with newline delimiter
- **Encoding**: UTF-8

### Message Delimiting

Messages are terminated with a newline character (`\n`, ASCII 10). This allows the receiver to detect complete messages.

```
Message 1 JSON\nMessage 2 JSON\n
```

### Handshake (Optional)

```
Host: {"type": "ping"}\n
MCU:  {"type": "pong", "firmware": "serial-json", "version": "1.0"}\n
Host: {"type": "config", "leds": 240, "protocol": "1.0"}\n
MCU:  {"type": "ready", "leds": 240, "buffer": 1024}\n
```

### Flow Control

For large pixel data, the host can send in chunks:

```json
{
  "type": "pixels",
  "data": [r,g,b,r,g,b,...],
  "offset": 0,
  "count": 240,
  "total": 240
}
```

The MCU responds with acknowledgment:

```json
{"type": "ack", "message": "pixels", "received": 240}
```

---

## I2C Protocol

### Overview

- **Transport**: I2C
- **Speed**: 400kHz (standard) or 1MHz (fast mode)
- **Format**: Binary
- **Address**: Configurable (default 0x08)

### Binary Message Format

```
Byte 0:     Message Type (1 byte)
Byte 1-2:   Payload Length (2 bytes, big-endian)
Byte 3..n:  Payload Data (variable)
```

### Message Types (I2C)

| Type ID | Name | Description |
|---------|------|-------------|
| 0x01 | PING | Keep-alive / connection test |
| 0x02 | PONG | Response to PING |
| 0x10 | PIXELS | RGB pixel data |
| 0x11 | PIXEL | Single pixel update |
| 0x20 | MODE | Set LED mode |
| 0x21 | BRIGHTNESS | Set brightness |
| 0x30 | CONFIG | Device configuration |
| 0x40 | STATUS | Status request |
| 0x41 | STATUS_RESP | Status response |
| 0xFF | ERROR | Error message |

### PIXELS Message (Type 0x10)

```
Byte 0:     0x10 (PIXELS)
Byte 1-2:   Length (N*3 + 4)
Byte 3-4:   Start index (uint16_t, big-endian)
Byte 5-6:   Count (uint16_t, big-endian)
Byte 7..n:  RGB data (count * 3 bytes)
```

Example: 3 pixels starting at index 0
```
0x10 0x0B 0x00 0x00 0x00 0x03 0xFF 0x00 0x00 0x00 0xFF 0x00 0xFF 0x00 0x00
```

---

## Message Types

### Common Message Types (All Protocols)

#### Connection Management

```json
// Client to Server
{"type": "identify", "client": "p5js", "version": "1.0"}

// Server to Client
{"type": "connected", "id": "client-123", "server": "fire-sticks-server"}

// Either direction
{"type": "ping"}
{"type": "pong"}
```

#### LED Control

```json
// Set all pixels
{
  "type": "pixels",
  "data": [255,0,0, 0,255,0, 0,0,255, ...],
  "count": 240,
  "offset": 0
}

// Set single pixel
{
  "type": "pixel",
  "index": 10,
  "color": [255, 128, 0]
}

// Set range of pixels
{
  "type": "pixelRange",
  "start": 0,
  "end": 99,
  "color": [255, 0, 0]
}

// Set mode
{
  "type": "mode",
  "mode": "fire|rainbow|solid|off|video"
}

// Set color (for solid mode)
{
  "type": "color",
  "color": [255, 100, 0]
}

// Set brightness
{
  "type": "brightness",
  "value": 128
}
```

#### Effects & Parameters

```json
// Fire effect parameters
{
  "type": "fireParams",
  "cooling": 55,
  "sparking": 120,
  "speed": 50
}

// Rainbow effect parameters
{
  "type": "rainbowParams",
  "speed": 50,
  "scale": 100
}
```

#### Video Mapping

```json
// Video frame data
{
  "type": "videoFrame",
  "width": 640,
  "height": 480,
  "data": "base64-encoded-image",
  "format": "jpeg|png|raw"
}

// Sample area for mapping
{
  "type": "sampleArea",
  "x": 100,
  "y": 100,
  "width": 240,
  "height": 1
}

// Mapping configuration
{
  "type": "mappingConfig",
  "sticks": [
    {"id": "stick1", "x": 0, "y": 0, "length": 240, "rotation": 0},
    {"id": "stick2", "x": 100, "y": 50, "length": 240, "rotation": 90}
  ]
}
```

#### Device Management

```json
// List connected devices
{
  "type": "listDevices"
}

// Response
{
  "type": "devices",
  "devices": [
    {"id": "esp32-1", "type": "esp32", "address": "192.168.1.100", "leds": 240},
    {"id": "esp32-2", "type": "esp32", "address": "192.168.1.101", "leds": 240}
  ]
}

// Device info
{
  "type": "deviceInfo",
  "id": "esp32-1",
  "firmware": "FireSticks",
  "version": "1.0",
  "leds": 240,
  "freeMemory": 12345,
  "uptime": 60
}
```

#### Status & Diagnostics

```json
// Status request
{"type": "status"}

// Status response
{
  "type": "status",
  "server": {
    "version": "1.0",
    "uptime": 1234,
    "connections": 5
  },
  "devices": [
    {"id": "esp32-1", "status": "connected", "lastSeen": 1234567890}
  ]
}

// Error message
{
  "type": "error",
  "code": 400,
  "message": "Invalid message format",
  "details": {"expected": "JSON", "received": "text/plain"}
}

// Log message
{
  "type": "log",
  "level": "info|warning|error",
  "message": "Connection established",
  "timestamp": 1234567890
}
```

---

## Error Handling

### Error Codes

| Code | Name | Description |
|------|------|-------------|
| 400 | BAD_REQUEST | Invalid message format |
| 401 | UNAUTHORIZED | Authentication required |
| 403 | FORBIDDEN | Access denied |
| 404 | NOT_FOUND | Resource not found |
| 408 | TIMEOUT | Request timeout |
| 413 | PAYLOAD_TOO_LARGE | Message too large |
| 429 | TOO_MANY_REQUESTS | Rate limit exceeded |
| 500 | INTERNAL_ERROR | Server error |
| 501 | NOT_IMPLEMENTED | Feature not available |
| 503 | SERVICE_UNAVAILABLE | Server busy |

### Error Response Format

```json
{
  "type": "error",
  "code": 400,
  "message": "Human readable error message",
  "timestamp": 1234567890,
  "requestId": "req-123",
  "details": {
    "field": "value",
    "expected": "type",
    "actual": "received"
  }
}
```

---

## Protocol Version History

### Version 1.0 (Current)

- Initial release
- Basic WebSocket and Serial JSON support
- Simple pixel data format
- Basic error handling

### Version 1.1 (Planned)

- Compression support for large pixel data
- Batch message support (multiple messages in one)
- Message prioritization (high/normal/low)
- Checksum/validation for critical messages

### Version 2.0 (Future)

- Binary protocol option for performance
- Encryption support
- Authentication and authorization
- Message sequencing and acknowledgment

---

## Testing Protocols

### WebSocket Testing

Use `wscat` to test WebSocket connections:

```bash
# Connect to server
wscat -c ws://localhost:8080

# Send a test message
> {"type": "ping"}

# Should receive: {"type": "pong"}
```

### Serial Testing

Use `screen` or `minicom` to test Serial connections:

```bash
# Connect to serial port
screen /dev/ttyUSB0 115200

# Send a test message (type and press Enter)
{"type": "ping"}

# Should receive: {"type": "pong", ...}
```

### I2C Testing

Use `i2c-tools` to test I2C connections:

```bash
# Scan for devices
i2cdetect -y 1

# Send bytes
i2cset -y 1 0x08 0x01 0x00 0x04

# Read bytes
i2cget -y 1 0x08
```

---

## Performance Considerations

### Message Size Limits

| Protocol | Max Message Size | Notes |
|----------|------------------|-------|
| WebSocket | 16KB (default) | Configurable, limited by memory |
| Serial | 4KB | Limited by buffer size |
| I2C | 32-255 bytes | Limited by I2C protocol |

### Throughput

| Protocol | Max Throughput | Latency |
|----------|----------------|---------|
| WebSocket (WiFi) | ~1-10 Mbps | 10-100ms |
| WebSocket (Ethernet) | ~10-100 Mbps | 1-10ms |
| Serial (115200) | ~115 Kbps | <1ms |
| Serial (2Mbps) | ~2 Mbps | <1ms |
| I2C (400kHz) | ~400 Kbps | <1ms |
| I2C (1MHz) | ~1 Mbps | <1ms |

### Recommendations

1. **For development**: Use WebSocket - easiest to debug
2. **For single device, tethered**: Use Serial - simplest, lowest latency
3. **For multiple devices, WiFi**: Use WebSocket via bridge server
4. **For low-latency, wired**: Use I2C or high-speed Serial
