/*
 * Automated Golf Ball Feeder
 *
 * Hardware:
 *   - Arduino Nano
 *   - Limit switch on D2 (INPUT_PULLUP - normally HIGH, LOW when pressed)
 *   - L298N Motor Driver: IN1=D5, IN2=D6
 *   - VL53L0X ToF Distance Sensor:
 *       - I2C: SDA=A4, SCL=A5
 *       - GPIO1 (interrupt) → D3
 *
 * Operation:
 *   - Object detected within 20cm → motor runs
 *   - Limit switch pressed → motor stops immediately
 *   - 10 second timeout safety if limit switch not triggered
 *
 * Library: Install "VL53L0X" by Pololu from Library Manager
 */

#include <Wire.h>
#include <VL53L0X.h>

// Pin definitions
const int PIN_LIMIT_SWITCH = 2;
const int PIN_SENSOR_INT = 3;   // VL53L0X GPIO1 interrupt pin
const int PIN_MOTOR_IN1 = 5;
const int PIN_MOTOR_IN2 = 6;

// Distance threshold (in mm)
const int TRIGGER_DISTANCE_MM = 200;  // 20cm = 200mm

// Timing constants
const unsigned long DEBOUNCE_DELAY_MS = 50;
const unsigned long MOTOR_TIMEOUT_MS = 10000;  // 10 second safety timeout
const unsigned long COOLDOWN_TIME_MS = 500;    // Time to ignore sensor after cycle

// Motor states
enum MotorState {
  MOTOR_STOPPED,
  MOTOR_RUNNING
};

// System states
enum SystemState {
  STATE_IDLE,
  STATE_FEEDING,
  STATE_COOLDOWN
};

// VL53L0X sensor
VL53L0X distanceSensor;
bool sensorInitialized = false;

// Global variables
volatile bool limitSwitchTriggered = false;
volatile bool sensorDataReady = false;
MotorState motorState = MOTOR_STOPPED;
SystemState systemState = STATE_IDLE;
unsigned long motorStartTime = 0;
unsigned long cooldownStartTime = 0;
bool waitingForLimitRelease = false;

// Debounce variables for limit switch
unsigned long lastLimitDebounceTime = 0;
int lastLimitReading = HIGH;
int limitSwitchState = HIGH;

// Distance sensor variables
int currentDistance = 9999;
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL_MS = 100;  // Fallback polling interval

void setup() {
  // Initialize serial for debugging
  Serial.begin(9600);
  Serial.println(F("Golf Feeder Starting..."));

  // Configure pins
  pinMode(PIN_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(PIN_SENSOR_INT, INPUT_PULLUP);  // GPIO1 is active LOW
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);

  // Ensure motor is stopped at startup
  stopMotor();

  // Initialize I2C and VL53L0X sensor
  Wire.begin();
  distanceSensor.setTimeout(500);

  if (!distanceSensor.init()) {
    Serial.println(F("ERROR: VL53L0X not found! Check wiring."));
    sensorInitialized = false;
  } else {
    Serial.println(F("VL53L0X initialized successfully"));
    sensorInitialized = true;

    // Configure for continuous mode with interrupt
    // Set GPIO1 to output interrupt on new data ready (active LOW)
    distanceSensor.writeReg(VL53L0X::SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);  // New sample ready
    distanceSensor.writeReg(VL53L0X::GPIO_HV_MUX_ACTIVE_HIGH,
                            distanceSensor.readReg(VL53L0X::GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);  // Active LOW

    // Clear any pending interrupt
    distanceSensor.writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR, 0x01);

    // Start continuous ranging (50ms between readings)
    distanceSensor.startContinuous(50);
  }

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(PIN_LIMIT_SWITCH), limitSwitchISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_INT), sensorISR, FALLING);

  Serial.println(F("System ready - waiting for object within 20cm..."));
}

void loop() {
  // Check for limit switch interrupt flag first
  if (limitSwitchTriggered) {
    handleLimitSwitch();
  }

  // Check for sensor data ready (interrupt or fallback polling)
  if (sensorDataReady || (millis() - lastSensorReadTime >= SENSOR_READ_INTERVAL_MS)) {
    // Also check if GPIO1 is LOW (data ready) in case we missed the edge
    if (sensorDataReady || digitalRead(PIN_SENSOR_INT) == LOW) {
      readDistanceSensor();
      lastSensorReadTime = millis();
    }
    sensorDataReady = false;
  }

  // Read limit switch state
  readLimitSwitch();

  // State machine
  switch (systemState) {
    case STATE_IDLE:
      handleIdleState();
      break;

    case STATE_FEEDING:
      handleFeedingState();
      break;

    case STATE_COOLDOWN:
      handleCooldownState();
      break;
  }
}

// ISR for limit switch
void limitSwitchISR() {
  limitSwitchTriggered = true;
}

// ISR for VL53L0X data ready
void sensorISR() {
  sensorDataReady = true;
}

// Handle limit switch trigger (called from main loop)
void handleLimitSwitch() {
  if (millis() - lastLimitDebounceTime > DEBOUNCE_DELAY_MS) {
    if (digitalRead(PIN_LIMIT_SWITCH) == LOW) {
      if (motorState == MOTOR_RUNNING && !waitingForLimitRelease) {
        Serial.println(F("Limit switch triggered - stopping motor"));
        stopMotor();
        enterCooldown();
      }
    }
    lastLimitDebounceTime = millis();
  }
  limitSwitchTriggered = false;
}

// Debounced limit switch reading
void readLimitSwitch() {
  int reading = digitalRead(PIN_LIMIT_SWITCH);

  if (reading != lastLimitReading) {
    lastLimitDebounceTime = millis();
  }

  if ((millis() - lastLimitDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != limitSwitchState) {
      limitSwitchState = reading;

      if (waitingForLimitRelease && limitSwitchState == HIGH) {
        waitingForLimitRelease = false;
        Serial.println(F("Limit switch released - now watching for press"));
      }
      else if (limitSwitchState == LOW && motorState == MOTOR_RUNNING && !waitingForLimitRelease) {
        Serial.println(F("Limit switch pressed - stopping motor"));
        stopMotor();
        enterCooldown();
      }
    }
  }

  lastLimitReading = reading;
}

// Read distance from sensor (called when interrupt fires)
void readDistanceSensor() {
  if (!sensorInitialized) {
    return;
  }

  // Read the distance
  uint16_t range = distanceSensor.readRangeContinuousMillimeters();

  // Clear the interrupt (write twice to ensure it clears)
  distanceSensor.writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR, 0x01);
  distanceSensor.writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR, 0x01);

  // Check for timeout/error (returns 65535 on error)
  if (distanceSensor.timeoutOccurred() || range > 8000) {
    currentDistance = 9999;
  } else {
    currentDistance = range;
  }
}

// Handle idle state - waiting for object detection
void handleIdleState() {
  if (!sensorInitialized) {
    return;
  }

  if (currentDistance < TRIGGER_DISTANCE_MM && currentDistance > 0) {
    Serial.print(F("Object detected at "));
    Serial.print(currentDistance);
    Serial.println(F("mm - starting feed cycle"));

    if (digitalRead(PIN_LIMIT_SWITCH) == LOW) {
      waitingForLimitRelease = true;
      Serial.println(F("Limit switch already pressed - waiting for release"));
    } else {
      waitingForLimitRelease = false;
    }

    startMotor();
    systemState = STATE_FEEDING;
    motorStartTime = millis();
  }
}

// Handle feeding state - motor running
void handleFeedingState() {
  if (millis() - motorStartTime >= MOTOR_TIMEOUT_MS) {
    Serial.println(F("TIMEOUT: Limit switch not triggered in 10 seconds - stopping motor"));
    stopMotor();
    enterCooldown();
    return;
  }
}

// Handle cooldown state - wait before accepting new triggers
void handleCooldownState() {
  if (millis() - cooldownStartTime >= COOLDOWN_TIME_MS) {
    if (currentDistance >= TRIGGER_DISTANCE_MM || currentDistance == 0) {
      Serial.println(F("Ready for next trigger"));
      systemState = STATE_IDLE;
    }
  }
}

// Enter cooldown after feed cycle completes
void enterCooldown() {
  systemState = STATE_COOLDOWN;
  cooldownStartTime = millis();
}

// Start motor
void startMotor() {
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  motorState = MOTOR_RUNNING;
  Serial.println(F("Motor started"));
}

// Stop motor immediately
void stopMotor() {
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  motorState = MOTOR_STOPPED;
  Serial.println(F("Motor stopped"));
}
