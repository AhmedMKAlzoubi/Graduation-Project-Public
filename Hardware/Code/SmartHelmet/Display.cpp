#include "Display.h"
#include <SPI.h> 

#define DIN    23
#define CLK    18
#define CS     5
#define RST    17 //TX2
#define DC     16 //RX2


//creats an object "display" to represent the screen and it's connections in my code
// to tell the library Adafruit_SSD1306 how is the screen connected and if it uses I2C OR SPi
Adafruit_SSD1306 display(128, 64, DIN, CLK, DC, RST, CS);

void initDisplay()
 {
  if(!display.begin(SSD1306_SWITCHCAPVCC)) 
  {  
    Serial.println("SSD1306 allocation failed");
    while(1); // stop here
  }
   else {
    Serial.println("HI");
   }
}


void Display(float voltage, int percent) 
{
  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("Voltage: ");
  display.print(voltage);
  display.println("V");
  display.println("Precentage: ");
  display.print(percent);
  display.println("%");


  display.display();
}