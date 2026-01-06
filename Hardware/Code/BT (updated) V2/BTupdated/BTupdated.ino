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
#define OLED_MOSI  23  // SDA/Data
#define OLED_CLK   18  // SCL/Clock
#define OLED_DC    16  // Data/Command
#define OLED_CS    5   // Chip Select
#define OLED_RESET 17  // Reset

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// BLE UUIDs - must match the app
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool wasConnected = false;  // Track previous connection state
String directionSymbol = "S";
int distanceMeters = 0;
String destination = "Waiting...";

// Forward declarations
void updateDisplay();
void drawArrow(String symbol);
String getDirectionText(String symbol);

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
      BLEDevice::startAdvertising(); // Restart advertising
    }
};

// BLE Characteristic Callbacks
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      
      if (value.length() > 0) {
        Serial.print("Received: ");
        Serial.println(value);
        
        // Check if data is in format "SYMBOL:DISTANCE" (e.g., "R:516")
        int colonIndex = value.indexOf(':');
        
        if (colonIndex > 0) {
          // New format: SYMBOL:DISTANCE
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
          // Old format: Full text instruction
          // Convert to lowercase for easier parsing
          String lowerValue = value;
          lowerValue.toLowerCase();
          
          // Parse direction from text
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
            // Extract exit number from roundabout instruction
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
          
          // Parse distance from text "(0.3 km)" or "(18 m)"
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
        
        // Update display immediately
        updateDisplay();
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Smart Helmet Starting...");

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

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE device is now advertising...");
  
  // Show waiting screen
  updateDisplay();
}

void loop() {
  // Main loop - display updates happen in onWrite callback
  delay(100);
}

void updateDisplay() {
  display.clearDisplay();
  
  // Add connection status indicator
  display.setTextSize(1);
  display.setCursor(100, 0);
  display.print(deviceConnected ? "BLE" : "---");
  
  if (!deviceConnected) {
    // Show disconnected or waiting status
    display.setTextSize(2);
    display.setCursor(0, 10);
    
    if (wasConnected) {
      // Was connected before, now disconnected
      display.println("Disconn-");
      display.println("ected");
    } else {
      // Never connected, waiting
      display.println("Waiting");
    }
    
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println("Connect via app");
    display.setCursor(0, 55);
    display.println("Smart-Helmet");
  } else {
    // Draw the arrow based on direction symbol
    drawArrow(directionSymbol);
    
    // Display distance
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
    
    // Display destination/instruction
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println("To destination:");
    display.setCursor(0, 55);
    display.println(getDirectionText(directionSymbol));
  }
  
  display.display();
}

void drawArrow(String symbol) {
  // Arrow drawing area: top-left corner (0-40, 0-40)
  int centerX = 20;
  int centerY = 20;
  
  if (symbol == "L") {
    // Left arrow ←
    display.fillTriangle(5, 20, 20, 10, 20, 30, SSD1306_WHITE);
    display.fillRect(20, 17, 15, 6, SSD1306_WHITE);
  }
  else if (symbol == "R") {
    // Right arrow →
    display.fillTriangle(35, 20, 20, 10, 20, 30, SSD1306_WHITE);
    display.fillRect(5, 17, 15, 6, SSD1306_WHITE);
  }
  else if (symbol == "S") {
    // Straight arrow ↑
    display.fillTriangle(20, 5, 10, 20, 30, 20, SSD1306_WHITE);
    display.fillRect(17, 20, 6, 15, SSD1306_WHITE);
  }
  else if (symbol == "U") {
    // U-turn (curved arrow)
    display.drawCircle(20, 15, 10, SSD1306_WHITE);
    display.fillTriangle(15, 25, 10, 30, 20, 30, SSD1306_WHITE);
  }
  else if (symbol.startsWith("RA")) {
    // Roundabout - draw circle with number
    display.drawCircle(20, 20, 15, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(14, 14);
    // Extract exit number (RA-2, RA-3, RA-4)
    display.print(symbol.substring(3));
    display.setTextSize(1);
  }
  else {
    // Default: straight arrow
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
