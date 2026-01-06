#include "Battery.h"

Adafruit_ADS1115 ADS;  //Creates an object "ADS" to represent the ADS115

const float R1 = 100000; //Resistor 1 100k ohms
const float R2 = 100000; //Resistor 2 100k ohms

const float Full = 4.2; //Voltage coming out of the battery when it is full
const float empty = 3; //Voltage coming out of the battery when  it is almost empty


// ADS1115 conversion constant for GAIN_ONE , gain one means 1 * 4.096
//4.096 is the ADS internal Ref voltage
const float Volt_Per_Count = 4.096 / 32768; // volts per count = 0.000125 V 
//ADS is a 16-bit analog-to-digital converter (ADC).
//it measures analog volatges andd converts it to a digitakl value between -32768 and + 32767 (2¹⁶ = 65536, 65536/2 = +- 32767 )
//since the gain is one, 1 * 4.096 = 4.096, and 32767 is the max, this means that 4.096 voltage is converted to +- 32767 
//therefoe voltage per bit is = 4.096/ 32767

void initBattery()
 {
  Serial.begin(115200);
  if (!ADS.begin()) 
  {
    Serial.println("ERROR: ADS1115 not found");
    while (1);
  }
  ADS.setGain(GAIN_ONE);
  Serial.println("ADS1115 initialized.");
}

float readBatteryVoltage() 
{
  int16_t Raw_Volatge = ADS.readADC_SingleEnded(0); // Reads a 16-bit signed integer fromt the ads channel A0 
  //(this is raw voltage count after the voltage divider )e.g: 4.2v is read as 32768 counts

  float Vnode = (float)Raw_Volatge * Volt_Per_Count; //(float)Raw_Volatge  converts the signed 16bit integers to float to acount for deciemals, 
  //(float)Raw_Volatge * Volt_Per_Count; multiplies the raw voltage count by the voltage per count "0.000125 V" to obtain the voltage as a total e.g :
  // Raw_Volatge = 25630 * Volt_Per_Count = 0.000125 V = 3.2
  //Vnode is the voltage at the Voltage divider node

  // Convert to actual battery voltage using divider formula
  float Vbattery = Vnode * ((R1 + R2) / R2);
  return Vbattery;
}

int voltageToPercent(float Voltage)
 {
  // Clamp to range
  if (Voltage >= Full )
  return 100;
  if (Voltage <= empty) 
  return 0;
  // Map linearly
  float Precentage = (Voltage - empty) / (Full - empty) * 100.0;
  return (int)roundf(Precentage);
}