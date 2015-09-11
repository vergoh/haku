#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define LCDBRIGHTNESS 0

// display object
Adafruit_AlphaNum4 lcd = Adafruit_AlphaNum4();

int scanindex = 0;

void setup() {
  Serial.begin(9600); 

  // set first display address
  lcd.begin(0x70);

  lcd.clear();
  lcd.setBrightness(LCDBRIGHTNESS);
  lcd.writeDisplay();

  showInfo();
}

void showInfo() {
  lcd.clear();
  lcd.writeDisplay();
  showText("VGO ");
  delay(1000);
  showText("RX  ");
  lcd.writeDigitAscii(2, '0', true);
  lcd.writeDigitAscii(3, '1');
  lcd.writeDisplay();
  delay(2000);
}

void showText(char *buffer) {
  if (strlen(buffer) != 4) {
    return;
  }

  for (int i=0; i<4; i++) {
    lcd.writeDigitAscii(i, buffer[i]);
  }
  lcd.writeDisplay();
}

void scanAnimation() {
  char t[5] = "SCAN";

  for (int i=0; i<4; i++) {
    if (scanindex == i) {
      lcd.writeDigitAscii(i, t[i], true);
    }    
    else {
      lcd.writeDigitAscii(i, t[i], false);
    }
  }
  lcd.writeDisplay();
  scanindex++;
  if (scanindex >= 4) {
    scanindex = 0; 
  }
}

void loop() {
  scanAnimation();
  delay(1000);
}










