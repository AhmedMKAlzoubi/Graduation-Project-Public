#include "Battery.h"

#include <Adafruit_ADS1X15.h>
extern Adafruit_ADS1115 ADS;


const float R1 = 100000; //Resistor 1 100k ohms
const float R2 = 100000; //Resistor 2 100k ohms

const float Full = 4.2; 
const float empty = 3; 


const float Volt_Per_Count = 4.096 / 32768; // volts per count = 0.000125 V 

void initializeBattery()
{

 Serial.begin(115200);

 if (!ADS.begin())
{
 Serial.println("ERROR: ADS1115 not found");
    while (1);
  }

   Serial.println("ADS initialized!");

}





float ReadBattery()
{
int16_t ReadRaw = ADS.readADC_SingleEnded(0);
Serial.println(ReadRaw);
float Vnode = (float)ReadRaw * Volt_Per_Count;

float Vbattery = Vnode * ((R1 + R2) / R2);
  return Vbattery;
}

int VoltageToPrecentage(float Vbattery)
{

 int percent = (int)((Vbattery - empty) / (Full - empty) * 100.0);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return percent;

}
