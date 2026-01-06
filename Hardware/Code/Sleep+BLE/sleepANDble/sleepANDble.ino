#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
#define HOLD_TIME 5000  // 10 seconds for power state change

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// BLE UUIDs
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool wasConnected = false;
String directionSymbol = "S";
int distanceMeters = 0;

RTC_DATA_ATTR int bootCount = 0;

// Forward declarations
void updateDisplay();
void drawArrow(String symbol);
String getDirectionText(String symbol);
bool waitForLongPress();
void enterDeepSleep();

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
          // Format: SYMBOL:DISTANCE
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
          // Parse text instruction
          String lowerValue = value;
          lowerValue.toLowerCase();
          
          if (lowerValue.indexOf("left") >= 0) {
            directionSymbol = "L";
          } 
          else if (lowerValue.indexOf("right") >= 0) {
            directionSymbol = "R";
          }
          else if (lowerValue.indexOf("straight") >= 0 || lowerValue.indexOf("merge") >= 0) {
            directionSymbol = "S";
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
          
          // Parse distance
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

  // Configure touch pin
  pinMode(TOUCH_PIN, INPUT);
  
  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up from deep sleep by touch sensor");
  }

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Helmet");
  display.println("Initializing...");
  display.display();
  delay(1000);

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
  Serial.println("Touch and hold for 10 seconds to enter sleep mode");
  
  updateDisplay();
}

void loop() {
  // Check for long press to enter deep sleep
  if (waitForLongPress()) {
    Serial.println("Long press detected - Entering deep sleep...");
    enterDeepSleep();
  }
  
  delay(100);
}

bool waitForLongPress() {
  if (digitalRead(TOUCH_PIN) == HIGH) {
    unsigned long pressStart = millis();
    Serial.println("Touch detected, hold for 10 seconds...");
    
    while (digitalRead(TOUCH_PIN) == HIGH) {
      unsigned long pressedTime = millis() - pressStart;
      
      if (pressedTime % 1000 < 100) {
        Serial.println("Held for " + String(pressedTime / 1000) + " seconds");
      }
      
      if (pressedTime >= HOLD_TIME) {
        Serial.println("10 second hold confirmed!");
        
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
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("Sleeping");
  display.display();
  delay(1000);
  
  Serial.println("Configuring deep sleep wake-up on GPIO " + String(TOUCH_PIN));
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_PIN, HIGH);
  
  Serial.println("Going to sleep now...");
  Serial.flush();
  delay(50);
  
  esp_deep_sleep_start();
}

void updateDisplay() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(100, 0);
  display.print(deviceConnected ? "BLE" : "---");
  
  if (!deviceConnected) {
    display.setTextSize(2);
    display.setCursor(0, 10);
    
    if (wasConnected) {
      display.println("Disconn-");
      display.println("ected");
    } else {
      display.println("Waiting");
    }
    
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println("Connect via app");
    display.setCursor(0, 55);
    display.println("Smart-Helmet");
  } else {
    drawArrow(directionSymbol);
    
    display.setTextSize(2);
    display.setCursor(50, 5);
    if (distanceMeters >= 1000) {
      float km = distanceMeters / 1000.0;
      display.print(km, 1);
      display.println(" km");
    } else {
      display.print(distanceMeters);
      display.println(" m");
    }
    
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println("To destination:");
    display.setCursor(0, 55);
    display.println(getDirectionText(directionSymbol));
  }
  
  display.display();
}

void drawArrow(String symbol) {
  int centerX = 20;
  int centerY = 20;
  
  if (symbol == "L") {
    display.fillTriangle(5, 20, 20, 10, 20, 30, SSD1306_WHITE);
    display.fillRect(20, 17, 15, 6, SSD1306_WHITE);
  }
  else if (symbol == "R") {
    display.fillTriangle(35, 20, 20, 10, 20, 30, SSD1306_WHITE);
    display.fillRect(5, 17, 15, 6, SSD1306_WHITE);
  }
  else if (symbol == "S") {
    display.fillTriangle(20, 5, 10, 20, 30, 20, SSD1306_WHITE);
    display.fillRect(17, 20, 6, 15, SSD1306_WHITE);
  }
  else if (symbol == "U") {
    display.drawCircle(20, 15, 10, SSD1306_WHITE);
    display.fillTriangle(15, 25, 10, 30, 20, 30, SSD1306_WHITE);
  }
  else if (symbol.startsWith("RA")) {
    display.drawCircle(20, 20, 15, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(14, 14);
    display.print(symbol.substring(3));
    display.setTextSize(1);
  }
  else {
    display.fillTriangle(20, 5, 10, 20, 30, 20, SSD1306_WHITE);
    display.fillRect(17, 20, 6, 15, SSD1306_WHITE);
  }
}

String getDirectionText(String symbol) {
  if (symbol == "L") return "Turn LEFT";
  if (symbol == "R") return "Turn RIGHT";
  if (symbol == "S") return "Continue STRAIGHT";
  if (symbol == "U") return "Make U-TURN";
  if (symbol.startsWith("RA")) return "Roundabout";
  return "Continue";
}
