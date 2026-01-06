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
#define TILT_THRESHOLD 45.0  // Degrees


// Battery monitoring - ADJUSTED FOR UNDER-LOAD READINGS
#define BATTERY_ADC_CHANNEL 0
const float R1 = 100000.0;
const float R2 = 100000.0;
const float BATTERY_FULL = 3.95;    // ← CHANGED: Full charge voltage UNDER LOAD through PB0A
const float BATTERY_EMPTY = 3.0;
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


// Charging animation
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL = 1000;
bool showThunderIcon = true;


// Tilt detection
unsigned long lastTiltCheck = 0;
const unsigned long TILT_CHECK_INTERVAL = 100;
float currentPitch = 0.0;
float currentRoll = 0.0;
bool tiltDetected = false;


// Forward declarations
void updateDisplay();
void drawArrow(String symbol);
void drawBatteryIcon(int x, int y, int percent, bool charging, bool showThunder);
void drawThunderIcon(int x, int y);
bool waitForLongPress();
void enterDeepSleep();
void updateBatteryReading();
float readBattery();
int voltageToPercentage(float vBattery);
void showWakeUpAnimation();
void centerText(String text, int y, int textSize);
void checkTilt();
void triggerVibration(int duration, int strength);


// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      wasConnected = true;
      Serial.println("Device connected");
      updateDisplay();
    }


    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
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


  // Configure pins
  pinMode(TOUCH_PIN, INPUT);
  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);
  
  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  bool wokeFromSleep = (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0);
  
  if (wokeFromSleep) {
    Serial.println("Woke up from deep sleep by touch sensor");
  }


  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("ERROR: MPU6050 not found");
  } else {
    Serial.println("MPU6050 initialized!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    delay(100);
  }


  // Initialize ADS1115 for battery monitoring
  if (!ADS.begin()) {
    Serial.println("ERROR: ADS1115 not found");
  } else {
    Serial.println("ADS1115 initialized!");
    ADS.setGain(GAIN_ONE);
  }


  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  Serial.println("SSD1306 allocation Success!");
  
  // Initial battery reading
  updateBatteryReading();
  previousBatteryVoltage = currentBatteryVoltage;
  
  // Show wake-up animation if woke from sleep
  if (wokeFromSleep) {
    showWakeUpAnimation();
  }


  // Initialize BLE
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
  // Check tilt every 100ms
  if (millis() - lastTiltCheck >= TILT_CHECK_INTERVAL) {
    checkTilt();
    lastTiltCheck = millis();
  }
  
  // Update battery reading periodically
  if (millis() - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
    updateBatteryReading();
    lastBatteryUpdate = millis();
  }
  
  // Handle charging animation blink
  if (isCharging && (millis() - lastBlinkTime >= BLINK_INTERVAL)) {
    showThunderIcon = !showThunderIcon;
    lastBlinkTime = millis();
    updateDisplay();
  }
  
  // Check for long press to enter deep sleep
  if (waitForLongPress()) {
    Serial.println("Long press detected - Entering deep sleep...");
    enterDeepSleep();
  }
  
  delay(50);
}


void checkTilt() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  currentPitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
  currentRoll = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
  
  if (abs(currentPitch) > TILT_THRESHOLD || abs(currentRoll) > TILT_THRESHOLD) {
    if (!tiltDetected) {
      tiltDetected = true;
      Serial.print("TILT DETECTED! Pitch: ");
      Serial.print(currentPitch);
      Serial.print(" Roll: ");
      Serial.println(currentRoll);
      
      triggerVibration(500, 255);
    }
  } else {
    tiltDetected = false;
  }
}


void triggerVibration(int duration, int strength) {
  analogWrite(VIBRATION_PIN, strength);
  delay(duration);
  analogWrite(VIBRATION_PIN, 0);
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
  // Take multiple samples and average to reduce noise
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
  
  // CALIBRATION: Correct for ADS1115 reference voltage error
  // Measured 2.1V actual but read 1.88V, so multiply by 1.117
  avgVoltage = avgVoltage * CALIBRATION_FACTOR;
  
  int16_t avgRaw = rawSum / 5;
  
  // DETAILED DEBUG INFO
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
  
  // More conservative charging detection (100mV threshold)
  if (currentBatteryVoltage > previousBatteryVoltage + 0.1) {
    isCharging = true;
  } else if (currentBatteryPercent >= 100) {
    isCharging = false;
  } else if (currentBatteryVoltage < previousBatteryVoltage - 0.05) {
    isCharging = false;
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
  int percent = (int)((vBattery - BATTERY_EMPTY) / (BATTERY_FULL - BATTERY_EMPTY) * 100.0);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return percent;
}


bool waitForLongPress() {
  if (digitalRead(TOUCH_PIN) == HIGH) {
    unsigned long pressStart = millis();
    Serial.println("Touch detected, hold for 5 seconds...");
    
    while (digitalRead(TOUCH_PIN) == HIGH) {
      unsigned long pressedTime = millis() - pressStart;
      
      if (pressedTime % 1000 < 100) {
        Serial.println("Held for " + String(pressedTime / 1000) + " seconds");
      }
      
      if (pressedTime >= HOLD_TIME) {
        Serial.println("5 second hold confirmed!");
        
        while (digitalRead(TOUCH_PIN) == HIGH) {
          delay(5);
        }
        delay(50);
        return true;
      }
      delay(50);
    }
    
    Serial.println("Released too early");
  }
  return false;
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
