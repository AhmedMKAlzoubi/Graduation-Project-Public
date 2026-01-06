#ifndef BATTERY_H
#define BATTERY_H

#include <Adafruit_ADS1X15.h>

void initBattery();
float readBatteryVoltage();
int voltageToPercent(float Voltage);

#endif
