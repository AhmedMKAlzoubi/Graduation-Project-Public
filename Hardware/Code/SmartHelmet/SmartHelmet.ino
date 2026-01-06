#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h> 

#include "Battery.h"
#include "Display.h"

void setup()
{
  Serial.begin(115200); // optional for debugging

  Wire.begin();

  initBattery();          // Initialize ADS1115
  initDisplay();

}

 void loop()
 {

  float Vbattery = readBatteryVoltage();
  int Precentage = voltageToPercent(Vbattery);

  Display(Vbattery, Precentage);


 delay(5000);

 }