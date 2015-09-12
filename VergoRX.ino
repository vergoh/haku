#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define LCDBRIGHTNESS 2

#define CS3PIN 10
#define CS2PIN 11
#define CS1PIN 12

#define MINCHAN 1
#define MAXCHAN 7

#define RSSIPIN               A6
#define RSSIMAX              560
#define RSSIMINLOCK    50
#define RSSINOISE          160
#define RSSISETTLETIME  200

#define KEEPLOCK          5
#define LOCKTESTDELAY 500

// display object
Adafruit_AlphaNum4 lcd = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 lcd2 = Adafruit_AlphaNum4();

int currentchan = 1;
int currentrssi = 0;
int scanindex = 0;
int chanindex = 0;
boolean rssilocked = false;
unsigned long locktime = 0;

const int chanfreq[] = { 
  5740, 5760, 5780, 5800, 5820, 5840, 5860 };

void setup()
{
  Serial.begin(9600); 

  pinMode(CS1PIN, OUTPUT);
  pinMode(CS2PIN, OUTPUT);
  pinMode(CS3PIN, OUTPUT);

  digitalWrite(CS1PIN, LOW);
  digitalWrite(CS2PIN, LOW);
  digitalWrite(CS3PIN, LOW);

  // init primary display
  lcd.begin(0x70);
  lcd.clear();
  lcd.setBrightness(LCDBRIGHTNESS);
  lcd.writeDisplay();

  // init secondary display
  lcd2.begin(0x71);
  lcd2.clear();
  lcd2.setBrightness(LCDBRIGHTNESS);
  lcd2.writeDisplay();

  showInfo();
}

void loop()
{
  checkRssiLock();
  if ( !rssilocked && !keepLock() ) {
    scanAnimation();
    changeChannel();
  }
  else {
    showChanInfo();
    delay(LOCKTESTDELAY);
  }
}

void showInfo()
{
  Serial.println("VergoRX 0.5");
  lcd.clear();
  lcd2.clear();
  lcd.writeDisplay();
  lcd2.writeDisplay();

  showText("VGO ");
  showText2("RX  ");
  lcd2.writeDigitAscii(2, '0', true);
  lcd2.writeDigitAscii(3, '5');
  lcd2.writeDisplay();
  delay(3000);
}

void showText(char *buffer)
{
  if (strlen(buffer) != 4) {
    return;
  }

  for (int i=0; i<4; i++) {
    lcd.writeDigitAscii(i, buffer[i]);
  }
  lcd.writeDisplay();
}

void showText2(char *buffer)
{
  if (strlen(buffer) != 4) {
    return;
  }

  for (int i=0; i<4; i++) {
    lcd2.writeDigitAscii(i, buffer[i]);
  }
  lcd2.writeDisplay();
}

void scanAnimation()
{
  char buffer[5];
  char anim[] = "-\\|/";

  snprintf(buffer, 5, "%c %02d", anim[scanindex], currentrssi);
  for (int i=0; i<4; i++) {
    lcd.writeDigitAscii(i, buffer[i]);
  }
  lcd.writeDisplay();
  scanindex++;
  if (scanindex >= 4) {
    scanindex = 0; 
  }
}

void showRSSIInfo(int rssi, int rssi_scaled)
{
  Serial.print("ch ");
  Serial.print(currentchan);
  Serial.print(" (");
  Serial.print(chanfreq[currentchan-1]);
  Serial.print(" MHz) rssi: ");
  Serial.print(rssi);
  Serial.print(" -> ");
  Serial.print(rssi_scaled);
  Serial.println("%");
}

int readRSSI()
{
  int rssi = 0;
  int rssi_scaled = 0;
  for (int i=0; i<10; i++) {
    rssi += analogRead(RSSIPIN);
    delay(20);
  }
  rssi = rssi / 10 - RSSINOISE;
  rssi_scaled = rssi / (float)(RSSIMAX - RSSINOISE) * 100;
  showRSSIInfo(rssi, rssi_scaled);
  return rssi_scaled;
}

void checkRssiLock()
{
  currentrssi = readRSSI();
  if (currentrssi >= RSSIMINLOCK) { 
    rssilocked = true;
    locktime = millis();
  } 
  else {
    rssilocked = false; 
  }
}

int keepLock()
{
  if (locktime == 0) {
    return 0; 
  }
  if (millis()-locktime > KEEPLOCK*1000) {
    return 0; 
  }
  return 1;
}

void setChannel(int channel)
{
  // LOW = ON
  switch (channel) {
  case 1:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, LOW);
    break;
  case 2:
    digitalWrite(CS1PIN, HIGH);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, LOW);
    break;
  case 3:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, HIGH);
    digitalWrite(CS3PIN, LOW);
    break;
  case 4:
    digitalWrite(CS1PIN, HIGH);
    digitalWrite(CS2PIN, HIGH);
    digitalWrite(CS3PIN, LOW);
    break;
  case 5:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, HIGH);
    break;
  case 6:
    digitalWrite(CS1PIN, HIGH);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, HIGH);
    break;
  case 7:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, HIGH);
    digitalWrite(CS3PIN, HIGH);
    break;
  default:
    break; 
  }
}

void changeChannel()
{
  char freq[5];

  currentchan++;
  if (currentchan > MAXCHAN) {
    currentchan = MINCHAN; 
  }
  setChannel(currentchan);

  snprintf(freq, 5, "%4d", chanfreq[currentchan-1]);
  showText2(freq);

  delay(RSSISETTLETIME);
}

void showChanInfo()
{
  char t[5];

  snprintf(t, 5, "%d %02d", currentchan, currentrssi);
  for (int i=0; i<4; i++) {
    if (i == 1) {
      lcd.writeDigitAscii(i, t[i], true);
    } 
    else{
      lcd.writeDigitAscii(i, t[i]);
    }
  }
  lcd.writeDisplay();
}


