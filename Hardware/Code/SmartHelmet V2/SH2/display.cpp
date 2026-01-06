#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define DIN    23
#define CLK    18
#define CS     5
#define RST    17 //TX2
#define DC     16 //RX2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, DC, RST, CS);

void initializeDisplay()
 {
  if(!display.begin(SSD1306_SWITCHCAPVCC)) 
  {  
    Serial.println("SSD1306 allocation failed!");
    while(1); // stop here
  }
   else {
    Serial.println("SSD1306 allocation Success1");
   }
}


