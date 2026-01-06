#include "Battery.h"
#include "Display.h"

Adafruit_ADS1115 ADS;

void setup()
{

  ADS.begin();

 initializeBattery();
 initializeDisplay();

  ADS.setGain(GAIN_ONE);

}

 void loop()
 {

  float Vbattery = ReadBattery();

  int Precentage = VoltageToPrecentage(Vbattery);

 Serial.println("Battery: ");
 Serial.print(Vbattery);
 Serial.println(" V \n");


 Serial.println("Precentage: ");
 Serial.print(Precentage);
 Serial.println(" %");
//\n
 delay(5000);

 }