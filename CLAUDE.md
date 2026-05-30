# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Arduino Nano-based automated golf ball dispenser that detects when a user is ready (via a time-of-flight distance sensor) and feeds a single ball on demand via a carousel mechanism. The system uses an interrupt-driven state machine for efficient operation and includes multiple safety features.

## Development Setup

### Arduino IDE Configuration

1. **Install Arduino IDE** and open `golf_feeder.ino`
2. **Install the VL53L0X library**:
   - Go to Sketch > Include Library > Manage Libraries
   - Search for "VL53L0X" by Pololu
   - Install the latest version
3. **Select Board**: Arduino Nano
4. **Select Processor**: ATmega328P (Old Bootloader) if upload fails
5. **Upload** to the board via USB

### Common Commands

- **Compile without upload**: Sketch > Verify/Compile (Ctrl+R)
- **Upload to board**: Sketch > Upload (Ctrl+U) or click the Upload button
- **Open Serial Monitor**: Tools > Serial Monitor (Ctrl+Shift+M, 9600 baud)
- **Clear Serial Monitor**: Ctrl+A in serial monitor, then Delete

## Architecture & Key Components

### State Machine (3 states)

- **STATE_IDLE**: Waiting for object detection within 200mm (20cm)
- **STATE_FEEDING**: Motor running, waiting for limit switch press or timeout
- **STATE_COOLDOWN**: Brief pause (500ms) to debounce before accepting next trigger

### Hardware Pins

| Component | Pin | Type | Notes |
|---|---|---|---|
| Limit Switch | D2 | INPUT_PULLUP | Normally HIGH, LOW when pressed (hardware interrupt) |
| VL53L0X Interrupt | D3 | INPUT_PULLUP | GPIO1 pin, active LOW, triggers on new distance data |
| Motor Driver IN1 | D5 | OUTPUT | Forward motion (LOW) |
| Motor Driver IN2 | D6 | OUTPUT | Backward/brake (HIGH for forward) |
| VL53L0X SDA | A4 | I2C | Distance sensor data line |
| VL53L0X SCL | A5 | I2C | Distance sensor clock line |

### Core Flow

1. **Idle state** continuously reads distance sensor via interrupt on D3
2. When object detected within 200mm: start motor, enter feeding state
3. Motor runs until either:
   - Limit switch (D2) is pressed → motor stops immediately
   - 10-second timeout → motor stops (safety)
4. Enter cooldown (500ms) to prevent false retriggers
5. Return to idle when object moves away

### Interrupt-Driven Design

- **Hardware interrupts** on both D2 (limit switch) and D3 (sensor) for immediate response
- **Interrupt flags** (`limitSwitchTriggered`, `sensorDataReady`) checked in main loop
- **Debouncing** applied in main loop (50ms) to filter electrical noise
- **Fallback polling** for distance sensor every 100ms if interrupt is missed

### Key Safety Features

- **10-second motor timeout**: Prevents jam damage if carousel gets stuck
- **Limit switch pre-press detection**: If already pressed at cycle start, system waits for release before watching for the trigger
- **Debounced inputs**: Both limit switch and distance readings filtered for 50ms stability
- **Dual interrupt handling**: Redundant checks in loop() for both interrupt and fallback polling

## Configurable Parameters

All parameters are defined as constants at the top of `golf_feeder.ino`:

```cpp
const int TRIGGER_DISTANCE_MM = 200;          // Detection distance in mm
const unsigned long MOTOR_TIMEOUT_MS = 10000; // Safety timeout in ms
const unsigned long COOLDOWN_TIME_MS = 500;   // Delay before next trigger
const unsigned long DEBOUNCE_DELAY_MS = 50;   // Input debounce window
const unsigned long SENSOR_READ_INTERVAL_MS = 100; // Fallback polling interval
```

Adjust these before uploading if tuning sensor sensitivity or timing.

## Serial Output for Debugging

The firmware sends status messages to Serial (9600 baud) including:
- Sensor initialization result
- Distance readings when object detected
- Motor start/stop events
- Limit switch state changes
- Timeout events
- State transitions

Use Serial Monitor to observe behavior during testing.

## Files in This Repository

- **golf_feeder.ino** — Main firmware (single sketch file)
- **golfballdispenser.STEP** — CAD model for 3D printing
- **Ball dispenser.pdf** — Detailed drawing with dimensions
- **Golf ball dispenser less than256 mm.3MF** — 3D model in Meshmix format
- **photos/** — Assembly photos showing wiring and construction
- **README.md** — Complete BOM, wiring diagram, and build instructions

## Modifying the Firmware

### Common Tasks

**Change detection distance**: Edit `TRIGGER_DISTANCE_MM` (value in mm, e.g., 150 for 15cm)

**Adjust motor timeout**: Edit `MOTOR_TIMEOUT_MS` (value in milliseconds, e.g., 5000 for 5 seconds)

**Tune debounce**: Edit `DEBOUNCE_DELAY_MS` if experiencing false triggers (increase) or sluggish response (decrease)

### Testing Changes

1. Modify parameters in the code
2. Verify compilation: Sketch > Verify/Compile
3. Upload: Sketch > Upload
4. Open Serial Monitor to watch behavior
5. Test with your hand or a club at various distances

## Known Hardware Considerations

- **VL53L0X sensor** works best with non-reflective surfaces; shiny metal may give erratic readings
- **Limit switch debounce**: Set to 50ms; mechanical switches may chatter
- **Motor inertia**: Even at 10 seconds, a stalled carousel may not complete its full rotation if jammed
- **12V power supply**: L298N motor driver requires solid 12V supply; brownout during motor spin can cause Arduino reset
