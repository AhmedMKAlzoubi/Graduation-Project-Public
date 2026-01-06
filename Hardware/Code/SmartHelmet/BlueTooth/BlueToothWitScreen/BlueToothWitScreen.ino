#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// ====== Global Constants and Objects ======
#define SERVICE_UUID           "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID_TX "19b10001-e8f2-537e-4f6c-d104768a1214"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI  23
#define OLED_CLK   18
#define OLED_DC    16
#define OLED_CS    5
#define OLED_RST   17

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);

// ====== Forward Declaration for drawArrow Function ======
void drawArrow(String dir);

// ====== Server Callback Class ======
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("BLE device connected");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("BLE device disconnected");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Disconnected");
    display.display();

    BLEDevice::startAdvertising(); // Restart BLE advertising
    Serial.println("Advertising restarted");
  }
};

// ====== Characteristic Callback Class ======
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue().c_str();
    if (value.length() > 0) {
      Serial.print("Received from app: ");
      Serial.println(value.c_str());

      // Parse format "DIR:distance", e.g. "N:100", "NE:56"
      int separatorIndex = value.indexOf(':');
      String dir = "";
      int distance = -1;
      if (separatorIndex > 0) {
        dir = value.substring(0, separatorIndex);
        distance = value.substring(separatorIndex + 1).toInt();
      } else {
        dir = value;
      }

      // Display direction, arrow, and distance on OLED
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("Dir: ");
      display.print(dir);

      // Draw arrow for direction
      drawArrow(dir);

      display.setCursor(0, 40);
      if (distance >= 0) {
        display.print("Dist: ");
        display.print(distance);
        display.print("m");
      }
      display.display();
    }
  }
};

// ====== Arrow Drawing Function ======
void drawArrow(String dir) {
  display.fillRect(90, 0, 38, 64, SSD1306_BLACK);

  if (dir == "N") {
    display.drawLine(109, 50, 109, 15, SSD1306_WHITE);
    display.drawLine(109, 15, 100, 25, SSD1306_WHITE);
    display.drawLine(109, 15, 118, 25, SSD1306_WHITE);
  } else if (dir == "NE") {
    display.drawLine(100, 50, 120, 30, SSD1306_WHITE);
    display.drawLine(120, 30, 110, 25, SSD1306_WHITE);
    display.drawLine(120, 30, 118, 40, SSD1306_WHITE);
  } else if (dir == "E") {
    display.drawLine(95, 40, 130, 40, SSD1306_WHITE);
    display.drawLine(130, 40, 120, 30, SSD1306_WHITE);
    display.drawLine(130, 40, 120, 50, SSD1306_WHITE);
  } else if (dir == "SE") {
    display.drawLine(100, 25, 120, 45, SSD1306_WHITE);
    display.drawLine(120, 45, 110, 50, SSD1306_WHITE);
    display.drawLine(120, 45, 118, 40, SSD1306_WHITE);
  } else if (dir == "S") {
    display.drawLine(109, 15, 109, 50, SSD1306_WHITE);
    display.drawLine(109, 50, 100, 40, SSD1306_WHITE);
    display.drawLine(109, 50, 118, 40, SSD1306_WHITE);
  } else if (dir == "SW") {
    display.drawLine(118, 25, 98, 45, SSD1306_WHITE);
    display.drawLine(98, 45, 108, 50, SSD1306_WHITE);
    display.drawLine(98, 45, 100, 40, SSD1306_WHITE);
  } else if (dir == "W") {
    display.drawLine(130, 40, 95, 40, SSD1306_WHITE);
    display.drawLine(95, 40, 105, 30, SSD1306_WHITE);
    display.drawLine(95, 40, 105, 50, SSD1306_WHITE);
  } else if (dir == "NW") {
    display.drawLine(120, 30, 100, 50, SSD1306_WHITE);
    display.drawLine(100, 50, 110, 55, SSD1306_WHITE);
    display.drawLine(100, 50, 98, 40, SSD1306_WHITE);
  } else {
    display.setCursor(90, 0);
    display.setTextSize(1);
    display.print("?");
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);

  // Initialize SPI bus with your custom pins
  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) { // no address for SPI!
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("SmartHelmet BLE Ready");
  display.display();

  BLEDevice::init("SmartHelmet");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  delay(1000);

  Serial.println("SmartHelmet BLE is ready. Waiting for app connection...");
}

// ====== Main Loop ======
void loop() {
  if (deviceConnected) {
    delay(1000);
  }
}
