# ESP32 I2C Slave (Placeholder)

**Note: This is a placeholder for future I2C slave implementation.**

## Purpose

For ultra-low-latency wired communication between multiple ESP32s and a master controller (e.g., Raspberry Pi, another ESP32, or Teensy).

## Why I2C?

- **Low Latency**: ~100kHz-1MHz clock speeds
- **Wired**: No WiFi overhead or interference
- **Multi-device**: Up to 127 devices on one bus
- **Simple**: Only 2 wires (SDA, SCL) + power/ground

## Current Status

This folder is a **placeholder**. The I2C slave implementation is not yet complete because:

1. ESP32 I2C slave support is still maturing in Arduino-ESP32
2. Binary protocol design needs finalization
3. Use cases need clarification (vs. Serial JSON and WebSocket)

## Alternative Options

For now, consider using:

1. **Serial JSON** - For single ESP32 connected via USB
2. **WebSocket** - For wireless multi-device control
3. **SPI** - For very high-speed communication (but more complex)

## Planned Implementation

The I2C slave firmware will:

- Use ESP32's I2C peripheral in slave mode
- Implement binary protocol for efficiency
- Support pixel data transfer
- Handle multiple commands
- Provide status feedback

### Binary Protocol (Draft)

```
Byte 0:     Message Type (1 byte)
Byte 1-2:   Payload Length (2 bytes, big-endian)
Byte 3..n:  Payload Data (variable)
```

#### Message Types

| Type ID (hex) | Name | Description |
|---------------|------|-------------|
| 0x01 | PING | Keep-alive |
| 0x02 | PONG | Response to PING |
| 0x10 | PIXELS | RGB pixel data |
| 0x11 | PIXEL | Single pixel update |
| 0x20 | MODE | Set LED mode |
| 0x21 | BRIGHTNESS | Set brightness |
| 0x30 | CONFIG | Device configuration |
| 0x40 | STATUS | Status request |
| 0x41 | STATUS_RESP | Status response |
| 0xFF | ERROR | Error message |

#### PIXELS Message

```
Byte 0:     0x10 (PIXELS)
Byte 1-2:   Length (N*3 + 4)
Byte 3-4:   Start index (uint16_t)
Byte 5-6:   Count (uint16_t)
Byte 7..n:  RGB data (count * 3 bytes)
```

## Hardware Requirements

- **I2C Bus**: SDA and SCL lines
- **Pull-up Resistors**: 4.7K on SDA and SCL (required for I2C)
- **Address Selection**: Each ESP32 needs unique I2C address

## Wiring

```
Master SDA ────┬──── ESP32-1 SDA
              │
Master SCL ────┴──── ESP32-1 SCL
              │
3.3V ─────────┴──── ESP32-1 VCC (I2C is 3.3V!)
              │
GND ──────────┴──── ESP32-1 GND
```

## See Also

- [Serial JSON Protocol](../serial-json/) - For USB Serial communication
- [WebSocket Client](../websocket-client/) - For WiFi communication
- [Protocols Documentation](../../docs/protocols.md) - For protocol specifications

## When to Use I2C

Consider I2C when:
- You need **ultra-low latency** (<1ms)
- You have **many devices** on one bus
- You want **wired reliability**
- You're using a **master controller** (RPi, Teensy, etc.)

Avoid I2C when:
- You need **long distances** (> few meters)
- You need **high throughput** (>100KB/s per device)
- You want **simple setup** (Serial is easier)

## License

MIT License
