# Golf Ball Dispenser

An automated golf ball dispenser that detects when you're ready and feeds a ball on demand. Built with 3D printed parts, an Arduino Nano, and off-the-shelf components.

![Top View](photos/top-view.jpg)
![Side View](photos/side-view.jpg)

## How It Works

1. A **VL53L0X Time-of-Flight laser distance sensor** detects when an object (your club or hand) is within 20cm
2. A **DC geared motor** spins a carousel that feeds a single golf ball to the exit chute
3. A **limit switch** detects when the carousel has completed one rotation and stops the motor
4. The system resets and waits for the next trigger

### Safety Features

- **10-second timeout**: Motor stops automatically if the limit switch isn't triggered within 10 seconds
- **Debounced inputs**: Both the limit switch and distance sensor readings are debounced to prevent false triggers
- **Interrupt-driven**: Limit switch uses hardware interrupt for immediate motor stop
- **Limit switch pre-press detection**: If the limit switch is already pressed when a cycle starts, the system waits for it to release before watching for the stop signal

## Wiring Diagram

| Component | Pin | Arduino Nano |
|---|---|---|
| Limit Switch | Signal | D2 |
| VL53L0X | GPIO1 (interrupt) | D3 |
| L298N | IN1 | D5 |
| L298N | IN2 | D6 |
| VL53L0X | SDA | A4 |
| VL53L0X | SCL | A5 |
| VL53L0X | VIN | 5V |
| VL53L0X | GND | GND |
| L298N | 12V | External 12V PSU |
| L298N | GND | Common GND |
| Limit Switch | COM | GND |
| Limit Switch | NO | D2 (uses internal pull-up) |

## Bill of Materials

> **Note:** Many of these parts are interchangeable and can be found on Amazon, eBay, or any electronics supplier. The AliExpress links below are what was used in this build. Prices shown are approximate per-unit costs (many listings sell in multi-packs, so buying just one may cost more).

### Electronics

| # | Component | Qty | Approx. Unit Price | Link |
|---|---|---|---|---|
| 1 | Arduino Nano (ATmega328P) | 1 | ~$2.50 | [AliExpress](https://www.aliexpress.us/item/3256806444439267.html) |
| 2 | L298N Motor Driver Module | 1 | ~$1.50 | [AliExpress](https://www.aliexpress.us/item/3256807078618738.html) |
| 3 | VL53L0X ToF Distance Sensor | 1 | ~$1.80 | [AliExpress](https://www.aliexpress.us/item/3256806179489186.html) |
| 4 | Low RPM DC Geared Motor (12V) | 1 | ~$3.00 | [AliExpress](https://www.aliexpress.us/item/3256805899293106.html) |
| 5 | Micro Limit Switch | 1 | ~$0.20 | [AliExpress](https://www.aliexpress.us/item/3256806027015669.html) |
| 6 | ON/OFF Rocker Switch | 1 | ~$0.30 | [AliExpress](https://www.aliexpress.us/item/2251832837763800.html) |
| 7 | DC Barrel Jack Panel Mount | 1 | ~$0.30 | [AliExpress](https://www.aliexpress.us/item/3256804296022885.html) |
| 8 | 12V 1A AC-DC Power Adapter | 1 | ~$3.00 | [AliExpress](https://www.aliexpress.us/item/3256804346592962.html) |
| 9 | Small Prototype PCB Board | 1 | ~$0.25 | [AliExpress](https://www.aliexpress.us/item/3256801518277095.html) |

### Hardware

| # | Component | Qty | Approx. Unit Price | Link |
|---|---|---|---|---|
| 10 | M5 Heat-Set Inserts (brass) | 7 | ~$0.10 ea | [AliExpress](https://www.aliexpress.us/item/3256809271320211.html) |
| 11 | M5 Screws | 7 | ~$0.05 ea | [AliExpress](https://www.aliexpress.us/item/3256802218134509.html) |
| 12 | M4 Screw + Nut | 1 | ~$0.05 | [AliExpress](https://www.aliexpress.us/item/3256802218134509.html) |
| 13 | M3 Screws | 2 | ~$0.05 ea | [AliExpress](https://www.aliexpress.us/item/3256802218134509.html) |
| 14 | 6mm Steel Ball Bearings / BBs | ~20+ | ~$0.02 ea | [AliExpress](https://www.aliexpress.us/item/3256805859138836.html) |
| 15 | Epoxy | 1 | ~$2.00 | - |

### Wiring / Consumables

| # | Component | Qty | Notes |
|---|---|---|---|
| 16 | JST XH Connectors | 1 set | [AliExpress](https://www.aliexpress.us/item/3256806179489186.html) |
| 17 | Hook-up Wire / Jumper Wire | assorted | Any silicone or dupont wire |
| 18 | Solder | - | Lead-free recommended |

**Estimated total cost: ~$18 - $25** (buying individual components)

> Prices fluctuate on AliExpress. Many of these components are sold in multi-packs, so the per-unit cost can be much lower if you're building multiple units or want spares.

### Tools Required

- **3D Printer** (for printing the enclosure and carousel)
- **Soldering iron** (for heat-set inserts and wiring)
- **Basic hand tools** (screwdrivers, wire strippers)

## 3D Printed Parts

The CAD model is provided as a STEP file (`golfballdispenser.STEP`) which can be opened in any CAD software (Fusion 360, FreeCAD, SolidWorks, etc.). A PDF drawing is also included (`Ball dispenser.pdf`).

### Print Settings (Recommended)

- **Material**: PLA or PETG
- **Layer Height**: 0.2mm
- **Infill**: 15-20%
- **Supports**: Yes (for the exit chute and motor mount)

## Software Setup

### Arduino IDE

1. Open `golf_feeder.ino` in Arduino IDE
2. Install the required library:
   - Go to **Sketch > Include Library > Manage Libraries**
   - Search for **"VL53L0X"** by **Pololu**
   - Install it
3. Select **Board**: Arduino Nano
4. Select **Processor**: ATmega328P (Old Bootloader) if upload fails
5. Upload

### Configurable Parameters

| Parameter | Default | Description |
|---|---|---|
| `TRIGGER_DISTANCE_MM` | 200 | Detection distance in mm (20cm) |
| `MOTOR_TIMEOUT_MS` | 10000 | Safety timeout in ms (10 seconds) |
| `COOLDOWN_TIME_MS` | 500 | Delay before accepting next trigger |
| `SENSOR_READ_INTERVAL_MS` | 100 | Fallback polling interval in ms |

## Photos

See the [photos/](photos/) directory for build photos showing the assembled unit, wiring, and internal components.

## License

This project is open source. Feel free to build, modify, and share.
