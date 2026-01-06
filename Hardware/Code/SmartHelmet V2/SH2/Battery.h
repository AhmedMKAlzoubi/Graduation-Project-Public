#ifndef BATTERY_H
#define BATTERY_H

#include <Wire.h>
#include <Adafruit_ADS1X15.h>


void initializeBattery();
float ReadBattery();
int VoltageToPrecentage(float Vbattery);

#endif