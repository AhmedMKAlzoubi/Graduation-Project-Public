#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h> 

void initDisplay();
void Display(float voltage, int percent);

#endif
