# CLAUDE.md - Golf Ball Dispenser Development Guide

## Project Overview

**Golf Ball Dispenser** is an automated dispenser system that uses a Time-of-Flight distance sensor to detect when a golfer is ready, then automatically feeds a single golf ball via a motor-driven carousel. Built with Arduino, 3D-printed parts, and off-the-shelf electronics components.

### Key Characteristics
- **Hardware-focused**: Arduino Nano microcontroller project
- **Embedded systems**: Real-time I2C sensor communication, interrupt-driven design
- **Safety-critical**: Timeout protection, debounced inputs, state machine architecture
- **Multi-domain**: Combines electronics (motor driver, sensors), firmware, and 3D CAD design

---

## Repository Structure

```
golf-ball-dispenser/
├── golf_feeder.ino                    # Main Arduino firmware
├── README.md                          # Complete user documentation
├── CLAUDE.md                          # This file - development guide
├── golfballdispenser.STEP             # CAD model (3D design)
├── Ball dispenser.pdf                 # Technical drawing
├── Golf ball dispenser less than256 mm.3MF  # Printable 3D model
├── photos/                            # Build photos (reference images)
├── .gitignore                         # Git exclusions
└── .git/                              # Version control

```

---

## Core Files & Responsibilities

### `golf_feeder.ino` - Main Firmware

**Purpose**: Arduino Nano firmware managing the entire dispenser operation

**Architecture**:
- **Hardware Layer**: Pin definitions, I2C/interrupt configuration
- **State Machine**: Three states (IDLE, FEEDING, COOLDOWN)
- **Interrupt Handlers**: Limit switch (D2) and sensor interrupt (D3) for responsive control
- **Sensor Integration**: VL53L0X ToF distance sensor via I2C with interrupt-driven data ready

**Key Functions**:
- `setup()`: Hardware initialization, I2C/sensor config, interrupt attachment
- `loop()`: Main state machine dispatcher
- `handleIdleState()`: Waits for object detection at <200mm
- `handleFeedingState()`: Motor running, monitors timeout (10s safety limit)
- `handleCooldownState()`: Debounce period after cycle completion
- `readDistanceSensor()`: Reads I2C sensor data and clears interrupt
- `readLimitSwitch()` / `limitSwitchISR()`: Debounced limit switch monitoring
- `startMotor()` / `stopMotor()`: Direct motor control via L298N driver

**Critical Details**:
- Uses hardware interrupts for immediate responsiveness
- Debounce timing: 50ms for switch, 500ms for cooldown
- Sensor timeout: 10 seconds before force-stopping motor
- Limit switch pre-release detection: Waits for switch to release if already pressed at cycle start
- Interrupt-driven sensor reading with fallback polling (100ms interval)

### `README.md` - Complete Documentation

**Purpose**: User-facing documentation for building, configuring, and operating the dispenser

**Sections**:
- How it works (component interaction)
- Safety features (timeout, debouncing, interrupts)
- Wiring diagram and BOM with AliExpress links
- Print settings for 3D parts
- Software setup (Arduino IDE configuration)
- Configurable parameters

**For AI Assistants**: Reference this for user documentation accuracy; don't modify without updating corresponding code.

### CAD & Design Files

**`golfballdispenser.STEP`**: Full 3D model in STEP format (universal CAD format)
- Contains carousel, enclosure, motor mounts, and exit chute
- Can be imported into Fusion 360, FreeCAD, SolidWorks, etc.

**`Ball dispenser.pdf`**: Technical drawing with dimensions and assembly notes

**`Golf ball dispenser less than256 mm.3MF`**: Slicable 3D print file

**Photos Directory**: Reference build images (not source code)

---

## Development Workflows

### Adding a New Feature

1. **Understand the current state machine** (lines 138-150) - identify where your feature fits
2. **Update hardware configuration** if new pins/sensors are needed (lines 24-27)
3. **Add timing constants** as needed near the top (lines 30-35)
4. **Implement state handling** in the appropriate `handle*State()` function
5. **Test in isolation** first (compile and upload with debug Serial output)
6. **Integrate with state transitions** and update the main loop if needed
7. **Add timeout/safety logic** - this is a safety-critical device

### Modifying Sensor Configuration

The VL53L0X sensor uses interrupt-driven continuous ranging:
- Interrupt pin (D3) triggers on new data ready (active LOW)
- I2C reads measurement data and clears interrupt
- Fallback polling every 100ms if interrupt missed
- Don't increase sensor frequency without testing timing impact

**When modifying**:
1. Update `distanceSensor.startContinuous()` timing (currently 50ms)
2. Verify interrupt clearing is still working (double-write pattern on line 215-216)
3. Check that debounce/timeout values still make sense
4. Test with Serial output: `Serial.print(F("Distance: ")); Serial.println(currentDistance);`

### Adjusting Safety Parameters

Key configurable values in firmware:
- **Line 30**: `TRIGGER_DISTANCE_MM` - Detection threshold (200mm = 20cm)
- **Line 34**: `MOTOR_TIMEOUT_MS` - Safety timeout (10000ms = 10 seconds)
- **Line 35**: `COOLDOWN_TIME_MS` - Debounce after cycle (500ms)
- **Line 71**: `SENSOR_READ_INTERVAL_MS` - Fallback polling (100ms)

Changes to these require:
1. Updating README.md "Configurable Parameters" table if user-facing
2. Testing on actual hardware before deployment
3. Documenting the rationale in commit message

---

## Code Conventions & Style

### Naming Conventions

**Pins & States**:
- Prefix hardware with `PIN_` (e.g., `PIN_MOTOR_IN1`)
- Use `UPPERCASE_SNAKE_CASE` for constants
- Use `camelCase` for variables and functions

**Volatile Variables**:
- Mark ISR-accessed variables as `volatile` (see lines 55-56)
- Document why in comments

**Booleans**:
- Use action verbs: `limitSwitchTriggered`, `sensorInitialized`, `waitingForLimitRelease`
- Avoid negations; use positive logic when possible

### Comments & Documentation

**Style**:
- One-line comments for **why**, not what
- Block comments for non-obvious logic (e.g., debounce algorithm)
- Use `F()` macro for all Serial strings to save RAM on ATmega328P (limited memory)

**Example**:
```cpp
// Wait for limit switch to physically release before watching for next press
if (waitingForLimitRelease && limitSwitchState == HIGH) {
  waitingForLimitRelease = false;
```

### Safety & Reliability

- **Always use timeout in motor-running state** - never rely solely on limit switch
- **Debounce all inputs** - mechanical switches bounce; 50ms is typical
- **Check sensor initialization** - gracefully handle VL53L0X failures
- **Use interrupt handlers only to set flags** - do work in main loop
- **Clear interrupts explicitly** - the VL53L0X requires double-write to clear (line 215-216)
- **Avoid busy-waiting** - use `millis()` for timing, not loops

---

## Testing & Verification

### Local Testing (Pre-Hardware)

1. **Compile Check**: Arduino IDE → Sketch → Verify (or Ctrl+R)
   - Catches syntax errors and missing includes
   - Shows RAM usage warnings if approaching ATmega328P limits (2KB RAM)

2. **Serial Debugging**: Upload and open Serial Monitor (9600 baud)
   - Verify initialization messages appear
   - Confirm sensor detection and state transitions
   - Watch for timeout messages

### Hardware Testing Checklist

- [ ] Sensor detects objects at correct distance (within ±50mm of 200mm)
- [ ] Motor starts immediately on detection
- [ ] Limit switch stops motor within 100ms
- [ ] Timeout triggers after 10 seconds if limit not pressed
- [ ] System resets properly after cooldown
- [ ] No false triggers from sensor noise
- [ ] Manual power cycle works correctly

### Regression Testing

If modifying state machine or timing:
1. Test normal flow: object → motor start → limit press → cooldown → idle
2. Test edge cases:
   - Limit switch already pressed at cycle start
   - Object present but too far (>200mm)
   - Sustained object presence (repeated triggers)
   - Sensor timeout scenario
   - Rapid repeated triggers

---

## Common Development Tasks

### Changing Detection Distance

1. **Update constant** (line 30):
   ```cpp
   const int TRIGGER_DISTANCE_MM = 150;  // 15cm instead of 20cm
   ```

2. **Update README.md** table in "Configurable Parameters" section

3. **Test on hardware** - calibrate with actual objects

### Adjusting Motor Control

Motor control uses L298N driver on pins D5 (IN1) and D6 (IN2):
- `startMotor()`: Sets IN1=LOW, IN2=HIGH (single direction only)
- `stopMotor()`: Sets both to LOW (coast/brake)
- Cannot reverse direction with current wiring

To modify motor behavior:
1. Check wiring diagram in README.md
2. Update `startMotor()` / `stopMotor()` functions only
3. Avoid PWM changes unless adding speed control (requires different approach)

### Adding Debug Output

The firmware uses `Serial.print()` and `Serial.println()` with the `F()` macro for RAM efficiency:
```cpp
Serial.println(F("Debug message here"));  // F() keeps string in PROGMEM, not RAM
```

Always use `F()` when adding Serial output - ATmega328P has only 2KB RAM.

### Modifying Interrupt Behavior

Interrupts are attached in `setup()` (lines 112-113):
```cpp
attachInterrupt(digitalPinToInterrupt(PIN_LIMIT_SWITCH), limitSwitchISR, FALLING);
attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_INT), sensorISR, FALLING);
```

Both trigger on FALLING edge. ISR functions only set flags; work happens in `loop()`.

**Do not**:
- Do heavy computation in ISRs
- Call `Serial.print()` from ISRs (can cause hangs)
- Use `delay()` in ISRs

---

## Git Workflow

### Branch Strategy

- **`main`** (or default): Stable, tested code
- **`claude/*`** (or feature branches): Development branches for new work
- Current development: `claude/claude-md-docs-5OdI3`

### Commit Message Format

Keep commits focused and atomic:
- **New feature**: "Add feature: description"
- **Bug fix**: "Fix: description of what was broken"
- **Docs update**: "Docs: update README section"
- **Refactoring**: "Refactor: improve naming/structure"

Example:
```
Fix: correct debounce timing for limit switch

Limit switch was triggering false positives due to insufficient 
debounce delay. Increased DEBOUNCE_DELAY_MS from 30ms to 50ms 
to match mechanical switch specs.
```

### Before Pushing

1. Verify code compiles without errors or warnings
2. Test on hardware if firmware changes
3. Update documentation if adding features or changing behavior
4. Commit with clear message describing the "why"

---

## Dependencies & Setup

### Arduino Environment

**Required**:
- Arduino IDE (latest version recommended)
- ATmega328P bootloader drivers (for Nano)

**Library Dependencies**:
- **VL53L0X by Pololu** (install via Library Manager)
  - Provides I2C communication and interrupt configuration
  - Used for Time-of-Flight distance sensing

### No External Build System

This is a single-file Arduino sketch with minimal dependencies:
- No build tool configuration needed
- No CI/CD pipeline (manual testing via hardware)
- No package manager beyond Arduino IDE's Library Manager

### Hardware Requirements for Testing

- Arduino Nano with ATmega328P
- VL53L0X distance sensor (I2C)
- L298N motor driver (or equivalent)
- Limit switch (mechanical)
- DC motor (12V, low RPM geared)
- Power supply matching hardware specifications (see README.md BOM)

---

## Important Considerations for AI Assistants

### Safety-Critical System

This device has moving parts and timeout mechanisms. **Any changes to timing, motor control, or sensor handling should be treated as safety-critical**:
- Always include timeout logic
- Never remove safety features (limit switch, timeout)
- Test thoroughly before recommending changes
- Flag any potential failure modes in code review

### Hardware Constraints

- **RAM**: ATmega328P has only 2KB - watch for large variable allocations
- **CPU Speed**: 16MHz - don't add computationally expensive operations
- **Flash**: 30KB usable for code and data
- **Pin Count**: Only 13 usable digital I/O pins
- Use `F()` macro for all Serial strings to conserve RAM

### Limited Testing Capability

Without physical hardware access:
- Can verify syntax and compilation
- Can reason about logic correctness
- Cannot test actual sensor readings, motor timing, or edge cases
- Recommend user run hardware tests for any firmware changes

### Documentation is Source of Truth

README.md is the official user documentation. When you find discrepancies between code and docs:
1. Verify which is correct by examining the actual implementation
2. Update the out-of-date one
3. Document the change in commit message

---

## Useful References

### Documentation
- README.md: Complete build and configuration guide
- Wiring Diagram: In README.md (Pin table)
- Bill of Materials: In README.md with supplier links
- Technical Drawing: Ball dispenser.pdf

### External Resources
- VL53L0X Library: https://github.com/pololu/vl53l0x-arduino
- Arduino Nano Pinout: https://www.arduino.cc/en/uploads/Main/ArduinoNano30SMD_REV3.pdf
- L298N Motor Driver Datasheet: https://en.wikipedia.org/wiki/L298
- ATmega328P Datasheet: For deep hardware behavior understanding

### Common Issues

**Sensor not initializing**:
- Check I2C wiring (SDA=A4, SCL=A5)
- Verify pull-up resistors if not built into board
- Try address scan sketch to confirm sensor on bus

**Limit switch not triggering motor stop**:
- Check PIN_LIMIT_SWITCH definition (should be D2)
- Verify switch wired to GND when pressed
- Test switch independently with multimeter

**Motor not responding**:
- Verify motor driver wiring (IN1=D5, IN2=D6)
- Check external 12V power supply
- Test motor directly with power supply

---

## Version History

- **v1.0** (Initial Release): Core functionality with safety features
  - Distance detection, motor control, limit switch integration
  - Debounced inputs, timeout protection
  - See git history for detailed changes

---

**Last Updated**: May 2026  
**Current Branch**: claude/claude-md-docs-5OdI3  
**Maintainer**: Claude Code AI Assistant
