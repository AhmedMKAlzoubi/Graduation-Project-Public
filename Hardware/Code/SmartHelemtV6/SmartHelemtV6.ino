#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// OLED Display settings (SPI)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI  23
#define OLED_CLK   18
#define OLED_DC    16
#define OLED_CS    5
#define OLED_RESET 17

// Touch sensor power control
#define TOUCH_PIN 32
#define HOLD_TIME 5000  // 5 seconds for power state change

// Vibration motor control
#define VIBRATION_PIN 25
#define TILT_THRESHOLD 40.0  // 40 degrees from calibrated position

// Battery monitoring - WITH 5% SAFETY BUFFER
#define BATTERY_ADC_CHANNEL 0
const float R1 = 100000.0;
const float R2 = 100000.0;

// ACTUAL battery voltages (including 5% buffer)
const float BATTERY_ABSOLUTE_MIN = 3.0;     // True empty (dangerous)
const float BATTERY_SAFE_MIN = 3.05;        // 5% buffer - show as "0%" to user
const float BATTERY_FULL = 3.95;            // Full charge under load

const float VOLT_PER_COUNT = 4.096 / 32768.0;
const float CALIBRATION_FACTOR = 1.117;  // Calibration: 2.1V actual / 1.88V measured

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_ADS1115 ADS;
Adafruit_MPU6050 mpu;

// BLE UUIDs
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool wasConnected = false;
String directionSymbol = "S";
int distanceMeters = 0;

RTC_DATA_ATTR int bootCount = 0;

// Battery update interval
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 5000;
float currentBatteryVoltage = 0.0;
int currentBatteryPercent = 0;
float previousBatteryVoltage = 0.0;
bool isCharging = false;

// Warning flags
bool lowBatteryWarningShown = false;
bool emptyBatteryWarningShown = false;

// Charging animation
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL = 1000;
bool showThunderIcon = true;

// Tilt detection with calibration
unsigned long lastTiltCheck = 0;
const unsigned long TILT_CHECK_INTERVAL = 100;
float currentPitch = 0.0;
float currentRoll = 0.0;
bool tiltDetected = false;

// MPU6050 calibration offsets (set at startup to current orientation)
float pitchOffset = 0.0;
float rollOffset = 0.0;

// Long press detection (non-blocking)
unsigned long touchPressStart = 0;
bool touchCurrentlyPressed = false;
bool longPressTriggered = false;

// Forward declarations
void updateDisplay();
void drawArrow(String symbol);
void drawBatteryIcon(int x, int y, int percent, bool charging, bool showThunder);
void drawThunderIcon(int x, int y);
void checkLongPress();
void enterDeepSleep();
void updateBatteryReading();
float readBattery();
int voltageToPercentage(float vBattery);
void showWakeUpAnimation();
void showBatteryEmptyWarning();
void centerText(String text, int y, int textSize);
void checkTilt();
void calibrateMPU6050();
void triggerVibration(int duration, int strength);
void triggerDirectionalVibration(String direction);

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    wasConnected = true;
    Serial.println("Device connected");
    
    // Clear navigation data when reconnecting
    directionSymbol = "S";
    distanceMeters = 0;
    
    updateDisplay();
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
    
    // Clear navigation data when disconnecting
    directionSymbol = "S";
    distanceMeters = 0;
    
    updateDisplay();
    BLEDevice::startAdvertising();
  }
};

// BLE Characteristic Callbacks
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();
    
    if (value.length() > 0) {
      Serial.print("Received: ");
      Serial.println(value);
      
      int colonIndex = value.indexOf(':');
      
      if (colonIndex > 0) {
        directionSymbol = value.substring(0, colonIndex);
        directionSymbol.trim();
        
        String distanceStr = value.substring(colonIndex + 1);
        distanceStr.trim();
        distanceMeters = distanceStr.toInt();
        
        Serial.print("Parsed - Symbol: ");
        Serial.print(directionSymbol);
        Serial.print(" | Distance: ");
        Serial.println(distanceMeters);
      }
      else {
        String lowerValue = value;
        lowerValue.toLowerCase();
        
        if (lowerValue.indexOf("left") >= 0) {
          directionSymbol = "L";
        } 
        else if (lowerValue.indexOf("right") >= 0) {
          directionSymbol = "R";
        }
        else if (lowerValue.indexOf("straight") >= 0) {
          directionSymbol = "S";
        }
        else if (lowerValue.indexOf("merge") >= 0) {
          if (lowerValue.indexOf("left") >= 0) {
            directionSymbol = "ML";
          } else if (lowerValue.indexOf("right") >= 0) {
            directionSymbol = "MR";
          } else {
            directionSymbol = "M";
          }
        }
        else if (lowerValue.indexOf("u-turn") >= 0 || lowerValue.indexOf("uturn") >= 0) {
          directionSymbol = "U";
        }
        else if (lowerValue.indexOf("roundabout") >= 0) {
          if (lowerValue.indexOf("2nd") >= 0) {
            directionSymbol = "RA-2";
          }
          else if (lowerValue.indexOf("3rd") >= 0) {
            directionSymbol = "RA-3";
          }
          else if (lowerValue.indexOf("4th") >= 0) {
            directionSymbol = "RA-4";
          }
          else {
            directionSymbol = "RA-1";
          }
        }
        else {
          directionSymbol = "S";
        }
        
        int openParen = value.indexOf('(');
        int closeParen = value.indexOf(')');
        
        if (openParen >= 0 && closeParen > openParen) {
          String distanceStr = value.substring(openParen + 1, closeParen);
          distanceStr.trim();
          
          if (distanceStr.indexOf("km") >= 0) {
            float km = distanceStr.substring(0, distanceStr.indexOf("km")).toFloat();
            distanceMeters = (int)(km * 1000);
          }
          else if (distanceStr.indexOf("m") >= 0) {
            distanceMeters = distanceStr.substring(0, distanceStr.indexOf("m")).toInt();
          }
        }
        
        Serial.print("Parsed - Symbol: ");
        Serial.print(directionSymbol);
        Serial.print(" | Distance: ");
        Serial.println(distanceMeters);
      }
      
      updateDisplay();
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(500);
  
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  Serial.println("Smart Helmet Starting...");

  pinMode(TOUCH_PIN, INPUT);
  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);
  
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool wokeFromSleep = (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0);
  
  if (wokeFromSleep) {
    Serial.println("Woke up from deep sleep by touch sensor");
  }

  if (!mpu.begin()) {
    Serial.println("ERROR: MPU6050 not found");
  } else {
    Serial.println("MPU6050 initialized!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    delay(100);
    calibrateMPU6050();
  }

  if (!ADS.begin()) {
    Serial.println("ERROR: ADS1115 not found");
  } else {
    Serial.println("ADS1115 initialized!");
    ADS.setGain(GAIN_ONE);
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  Serial.println("SSD1306 allocation Success!");
  
  updateBatteryReading();
  previousBatteryVoltage = currentBatteryVoltage;
  
  // Show animation on every boot (power-on and wake from sleep)
  showWakeUpAnimation();

  BLEDevice::init("Smart-Helmet");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE device is now advertising...");
  Serial.println("Touch and hold for 5 seconds to enter sleep mode");
  
  updateDisplay();
}

void loop() {
  if (millis() - lastTiltCheck >= TILT_CHECK_INTERVAL) {
    checkTilt();
    lastTiltCheck = millis();
  }
  
  if (millis() - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
    updateBatteryReading();
    lastBatteryUpdate = millis();
  }
  
  if (isCharging && (millis() - lastBlinkTime >= BLINK_INTERVAL)) {
    showThunderIcon = !showThunderIcon;
    lastBlinkTime = millis();
    updateDisplay();
  }
  
  checkLongPress();
  
  delay(10);
}

void calibrateMPU6050() {
  Serial.println("Calibrating MPU6050 to current orientation...");
  Serial.println("Keep helmet still for 2 seconds...");
  
  float pitchSum = 0;
  float rollSum = 0;
  int samples = 20;
  
  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
    float roll = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
    
    pitchSum += pitch;
    rollSum += roll;
    
    delay(100);
  }
  
  pitchOffset = pitchSum / samples;
  rollOffset = rollSum / samples;
  
  Serial.print("Calibration complete! Offsets - Pitch: ");
  Serial.print(pitchOffset);
  Serial.print("° Roll: ");
  Serial.print(rollOffset);
  Serial.println("°");
}

void checkTilt() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  float rawPitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
  float rawRoll = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
  
  // Apply calibration to get relative angles from startup position
  currentPitch = rawPitch - pitchOffset;
  currentRoll = rawRoll - rollOffset;
  
  // Detect tilt direction and magnitude
  bool pitchTiltForward = currentPitch > TILT_THRESHOLD;
  bool pitchTiltBackward = currentPitch < -TILT_THRESHOLD;
  bool rollTiltRight = currentRoll > TILT_THRESHOLD;
  bool rollTiltLeft = currentRoll < -TILT_THRESHOLD;
  
  // Check if any tilt exceeds threshold
  if (pitchTiltForward || pitchTiltBackward || rollTiltRight || rollTiltLeft) {
    if (!tiltDetected) {
      tiltDetected = true;
      
      Serial.print("TILT DETECTED! Pitch: ");
      Serial.print(currentPitch);
      Serial.print("° Roll: ");
      Serial.print(currentRoll);
      Serial.print("° | Direction: ");
      
      // Determine primary tilt direction
      String tiltDirection = "";
      
      if (abs(currentPitch) > abs(currentRoll)) {
        // Pitch is dominant
        if (pitchTiltForward) {
          tiltDirection = "FORWARD";
        } else {
          tiltDirection = "BACKWARD";
        }
      } else {
        // Roll is dominant
        if (rollTiltRight) {
          tiltDirection = "RIGHT";
        } else {
          tiltDirection = "LEFT";
        }
      }
      
      Serial.println(tiltDirection);
      
      // Trigger directional vibration pattern
      triggerDirectionalVibration(tiltDirection);
    }
  } else {
    tiltDetected = false;
  }
}

void triggerDirectionalVibration(String direction) {
  if (direction == "FORWARD") {
    // 1 long pulse
    analogWrite(VIBRATION_PIN, 255);
    delay(500);
    analogWrite(VIBRATION_PIN, 0);
  }
  else if (direction == "BACKWARD") {
    // 2 medium pulses
    analogWrite(VIBRATION_PIN, 255);
    delay(250);
    analogWrite(VIBRATION_PIN, 0);
    delay(100);
    analogWrite(VIBRATION_PIN, 255);
    delay(250);
    analogWrite(VIBRATION_PIN, 0);
  }
  else if (direction == "LEFT") {
    // 3 short pulses
    for (int i = 0; i < 3; i++) {
      analogWrite(VIBRATION_PIN, 255);
      delay(150);
      analogWrite(VIBRATION_PIN, 0);
      delay(100);
    }
  }
  else if (direction == "RIGHT") {
    // 4 very short pulses
    for (int i = 0; i < 4; i++) {
      analogWrite(VIBRATION_PIN, 255);
      delay(100);
      analogWrite(VIBRATION_PIN, 0);
      delay(80);
    }
  }
}

void triggerVibration(int duration, int strength) {
  analogWrite(VIBRATION_PIN, strength);
  delay(duration);
  analogWrite(VIBRATION_PIN, 0);
}

void checkLongPress() {
  bool touchState = digitalRead(TOUCH_PIN);
  
  if (touchState == HIGH && !touchCurrentlyPressed) {
    touchCurrentlyPressed = true;
    touchPressStart = millis();
    longPressTriggered = false;
    Serial.println("Touch detected, hold for 5 seconds...");
  }
  
  if (touchState == HIGH && touchCurrentlyPressed && !longPressTriggered) {
    unsigned long pressedTime = millis() - touchPressStart;
    
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime >= 1000) {
      Serial.println("Held for " + String(pressedTime / 1000) + " seconds");
      lastPrintTime = millis();
    }
    
    if (pressedTime >= HOLD_TIME) {
      Serial.println("5 second hold confirmed!");
      longPressTriggered = true;
      Serial.println("Release touch to enter sleep...");
    }
  }
  
  if (touchState == LOW && touchCurrentlyPressed) {
    unsigned long pressedTime = millis() - touchPressStart;
    touchCurrentlyPressed = false;
    
    if (longPressTriggered) {
      Serial.println("Entering deep sleep...");
      enterDeepSleep();
    } else {
      Serial.println("Released after " + String(pressedTime) + "ms (too early)");
    }
  }
}

void showWakeUpAnimation() {
  for (int cycle = 0; cycle < 3; cycle++) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    centerText("Smart", 24, 2);
    centerText("Helmet", 42, 2);
    display.display();
    delay(400);
    
    display.clearDisplay();
    display.display();
    delay(200);
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  centerText("Smart", 24, 2);
  centerText("Helmet", 42, 2);
  display.display();
  delay(1000);
}

void showBatteryEmptyWarning() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.drawRect(40, 10, 48, 24, SSD1306_WHITE);
  display.fillRect(88, 16, 8, 12, SSD1306_WHITE);
  
  display.drawLine(45, 15, 83, 29, SSD1306_WHITE);
  display.drawLine(83, 15, 45, 29, SSD1306_WHITE);
  
  centerText("Battery Empty!", 40, 1);
  centerText("Charge to avoid", 52, 1);
  centerText("damage!", 62, 1);
  
  display.display();
}

void centerText(String text, int y, int textSize) {
  int16_t x1, y1;
  uint16_t w, h;
  
  display.setTextSize(textSize);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

float readBattery() {
  float sum = 0;
  int16_t rawSum = 0;
  
  for (int i = 0; i < 5; i++) {
    int16_t readRaw = ADS.readADC_SingleEnded(BATTERY_ADC_CHANNEL);
    rawSum += readRaw;
    
    float vNode = (float)readRaw * VOLT_PER_COUNT;
    float vBattery = vNode * ((R1 + R2) / R2);
    sum += vBattery;
    delay(10);
  }
  
  float avgVoltage = sum / 5.0;
  avgVoltage = avgVoltage * CALIBRATION_FACTOR;
  
  int16_t avgRaw = rawSum / 5;
  
  Serial.println("========== BATTERY DEBUG ==========");
  Serial.print("RAW ADC Value: ");
  Serial.println(avgRaw);
  Serial.print("Voltage at A0 (Vnode): ");
  Serial.print(avgVoltage / 2.0);
  Serial.println(" V");
  Serial.print("Calculated Battery Voltage: ");
  Serial.print(avgVoltage);
  Serial.println(" V");
  Serial.println("===================================");
  
  return avgVoltage;
}

void updateBatteryReading() {
  currentBatteryVoltage = readBattery();
  currentBatteryPercent = voltageToPercentage(currentBatteryVoltage);
  
  if (currentBatteryVoltage > previousBatteryVoltage + 0.1) {
    isCharging = true;
    lowBatteryWarningShown = false;
    emptyBatteryWarningShown = false;
  } else if (currentBatteryPercent >= 100) {
    isCharging = false;
  } else if (currentBatteryVoltage < previousBatteryVoltage - 0.05) {
    isCharging = false;
  }
  
  if (currentBatteryPercent <= 20 && !isCharging && !lowBatteryWarningShown) {
    Serial.println("⚠️ LOW BATTERY WARNING!");
    lowBatteryWarningShown = true;
    
    triggerVibration(150, 200);
    delay(200);
    triggerVibration(150, 200);
  }
  
  if (currentBatteryPercent <= 0 && !isCharging && !emptyBatteryWarningShown) {
    Serial.println("🔴 BATTERY EMPTY! Charge to avoid permanent damage!");
    emptyBatteryWarningShown = true;
    
    triggerVibration(300, 255);
    delay(300);
    triggerVibration(300, 255);
    delay(300);
    triggerVibration(300, 255);
    
    showBatteryEmptyWarning();
    
    delay(10000);
    enterDeepSleep();
  }
  
  Serial.print("Battery: ");
  Serial.print(currentBatteryVoltage);
  Serial.print(" V (");
  Serial.print(currentBatteryPercent);
  Serial.print("%) ");
  Serial.println(isCharging ? "CHARGING" : "DISCHARGING");
  
  previousBatteryVoltage = currentBatteryVoltage;
}

int voltageToPercentage(float vBattery) {
  int percent = (int)((vBattery - BATTERY_SAFE_MIN) / (BATTERY_FULL - BATTERY_SAFE_MIN) * 100.0);
  
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  
  return percent;
}

void enterDeepSleep() {
  display.clearDisplay();
  display.display();
  delay(100);
  
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  
  Serial.println("Display turned off");
  Serial.println("Configuring deep sleep wake-up on GPIO " + String(TOUCH_PIN));
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_PIN, HIGH);
  
  Serial.println("Going to sleep now...");
  Serial.flush();
  delay(50);
  
  esp_deep_sleep_start();
}

void updateDisplay() {
  display.clearDisplay();
  
  drawBatteryIcon(0, 0, currentBatteryPercent, isCharging, showThunderIcon);
  display.setTextSize(1);
  display.setCursor(18, 1);
  display.print(currentBatteryPercent);
  display.print("%");
  
  if (!deviceConnected) {
    display.setTextSize(1);
    centerText("Bluetooth is", 25, 1);
    centerText("disconnected", 35, 1);
    
  } else {
    if (distanceMeters == 0 && directionSymbol == "S") {
      display.setTextSize(1);
      centerText("Bluetooth is", 15, 1);
      centerText("connected", 25, 1);
      centerText("Waiting for", 40, 1);
      centerText("instructions", 50, 1);
    } else {
      drawArrow(directionSymbol);
      
      display.setTextSize(2);
      display.setCursor(50, 5);
      if (distanceMeters >= 1000) {
        float km = distanceMeters / 1000.0;
        display.print(km, 1);
        display.println("km");
      } else {
        display.print(distanceMeters);
        display.println("m");
      }
    }
  }
  
  display.display();
}

void drawBatteryIcon(int x, int y, int percent, bool charging, bool showThunder) {
  display.drawRect(x, y + 2, 12, 6, SSD1306_WHITE);
  display.fillRect(x + 12, y + 3, 2, 4, SSD1306_WHITE);
  
  if (charging && showThunder) {
    drawThunderIcon(x + 3, y + 3);
  } else if (charging && !showThunder) {
    int fillWidth = map(percent, 0, 100, 0, 10);
    if (fillWidth > 0) {
      display.fillRect(x + 1, y + 3, fillWidth, 4, SSD1306_WHITE);
    }
  } else {
    int fillWidth = map(percent, 0, 100, 0, 10);
    if (fillWidth > 0) {
      display.fillRect(x + 1, y + 3, fillWidth, 4, SSD1306_WHITE);
    }
  }
  
  if (!charging && percent < 20) {
    display.drawRect(x, y + 2, 12, 6, SSD1306_WHITE);
  }
}

void drawThunderIcon(int x, int y) {
  display.fillTriangle(x + 3, y, x, y + 3, x + 2, y + 3, SSD1306_WHITE);
  display.fillTriangle(x, y + 3, x + 3, y + 6, x + 1, y + 3, SSD1306_WHITE);
}

void drawArrow(String symbol) {
  int centerX = 20;
  int centerY = 30;

  display.fillRect(0, 10, 40, 44, SSD1306_BLACK);

  if (symbol == "L") {
    display.fillRect(10, centerY - 2, 18, 4, SSD1306_WHITE);
    display.fillTriangle(10, centerY, 16, centerY - 6, 16, centerY + 6, SSD1306_WHITE);
  }
  else if (symbol == "R") {
    display.fillRect(10, centerY - 2, 18, 4, SSD1306_WHITE);
    display.fillTriangle(28, centerY, 22, centerY - 6, 22, centerY + 6, SSD1306_WHITE);
  }
  else if (symbol == "S") {
    display.fillRect(centerX - 2, 18, 4, 20, SSD1306_WHITE);
    display.fillTriangle(centerX, 12, centerX - 8, 22, centerX + 8, 22, SSD1306_WHITE);
  }
  else if (symbol == "U") {
    display.fillRect(centerX - 2, 14, 4, 10, SSD1306_WHITE);
    display.fillRect(centerX - 12, 24, 24, 4, SSD1306_WHITE);
    display.fillRect(centerX - 12, 14, 4, 10, SSD1306_WHITE);
    display.fillTriangle(centerX - 12, 14, centerX - 16, 20, centerX - 8, 20, SSD1306_WHITE);
  }
  else if (symbol == "M" || symbol == "ML") {
    display.fillRect(centerX + 2, 14, 4, 24, SSD1306_WHITE);
    display.drawLine(centerX - 10, 14, centerX, 30, SSD1306_WHITE);
    display.drawLine(centerX - 9, 14, centerX + 1, 30, SSD1306_WHITE);
  }
  else if (symbol == "MR") {
    display.fillRect(centerX - 6, 14, 4, 24, SSD1306_WHITE);
    display.drawLine(centerX + 10, 14, centerX, 30, SSD1306_WHITE);
    display.drawLine(centerX + 9, 14, centerX - 1, 30, SSD1306_WHITE);
  }
  else if (symbol.startsWith("RA")) {
    display.drawCircle(centerX, centerY, 10, SSD1306_WHITE);

    if (symbol.length() > 3) {
      String num = symbol.substring(3);
      display.setTextSize(1);
      display.setCursor(centerX - 3, centerY - 3);
      display.print(num);
    }
  }
  else {
    display.fillRect(centerX - 2, 18, 4, 20, SSD1306_WHITE);
    display.fillTriangle(centerX, 12, centerX - 8, 22, centerX + 8, 22, SSD1306_WHITE);
  }
}
