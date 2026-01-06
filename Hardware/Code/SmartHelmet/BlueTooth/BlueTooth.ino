#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID_TX "19b10001-e8f2-537e-4f6c-d104768a1214"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE device connected");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE device disconnected");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
   String value = pCharacteristic->getValue().c_str();
    if (value.length() > 0) {
      Serial.print("Received from app: ");
      Serial.println(value.c_str());
    }
  }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("SmartHelmet"); // Same name shown on phone
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
  BLEDevice::startAdvertising();

  Serial.println("SmartHelmet BLE is ready. Waiting for app connection...");
}

void loop() {
  if (deviceConnected) {
    // Example: send feedback to app
    pCharacteristic->setValue("ESP32 active");
    pCharacteristic->notify();
    delay(2000);
  }
}