// ESP32 Deep Sleep with TTP223 Touch Sensor Control
#define TOUCH_PIN 32         // GPIO where TTP223 output is connected
#define HOLD_TIME 5000     // 5 seconds in milliseconds

RTC_DATA_ATTR int bootCount = 0;  // Stored in RTC memory to survive deep sleep

void setup() {
  Serial.begin(115200);
  delay(500);
  
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  
  // Configure touch pin as input
  pinMode(TOUCH_PIN, INPUT);
  
  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up from deep sleep by touch sensor");
  } else {
    Serial.println("Normal startup or other wake-up source");
  }
  
  // Wait and check if touch sensor is held for 5 seconds
  Serial.println("Touch and hold the sensor for 5 seconds to enter/exit sleep mode...");
  
  if (waitForLongPress()) {
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
      Serial.println("Long press detected - Staying awake!");
      Serial.println("System is now active. Hold again for 5 seconds to sleep.");
    } else {
      Serial.println("Long press detected - Entering deep sleep...");
      enterDeepSleep();
    }
  }
}

void loop() {
  // Main program runs here when awake
  Serial.println("ESP32 is awake and running...");
  delay(2000);
  
  // Continuously check for long press to enter sleep
  if (waitForLongPress()) {
    Serial.println("Long press detected - Entering deep sleep...");
    enterDeepSleep();
  }
}

bool waitForLongPress() {
  // Check if sensor is currently touched
  if (digitalRead(TOUCH_PIN) == HIGH) {
    unsigned long pressStart = millis();
    Serial.println("Touch detected, hold for 5 seconds...");
    
    // Wait and verify continuous touch
    while (digitalRead(TOUCH_PIN) == HIGH) {
      unsigned long pressedTime = millis() - pressStart;
      
      // Print progress every second
      if (pressedTime % 500 < 50) {
        Serial.println("Held for " + String(pressedTime / 500) + " seconds");
      }
      
      // Check if 5 seconds elapsed
      if (pressedTime >= HOLD_TIME) {
        Serial.println("5 second hold confirmed!");
        
        // Wait for release
        while (digitalRead(TOUCH_PIN) == HIGH) {
          delay(5);
        }
        delay(50); // Debounce
        return true;
      }
      delay(50);
    }
    
    // Released before 5 seconds
    Serial.println("Released too early, hold not registered");
  }
  return false;
}

void enterDeepSleep() {
  Serial.println("Configuring deep sleep wake-up on GPIO " + String(TOUCH_PIN));
  
  // Configure ext0 wake-up: wake when TTP223 goes HIGH
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_PIN, HIGH);
  
  Serial.println("Going to sleep now...");
  Serial.flush();
  delay(50);
  
  // Enter deep sleep
  esp_deep_sleep_start();
}
