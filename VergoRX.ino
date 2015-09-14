#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define LCDBRIGHTNESS 2

#define BS1PIN 8
#define BS2PIN 9

// note that pin order in receiver
// is inverse compared to transmitter
#define CS1PIN 10
#define CS2PIN 11
#define CS3PIN 12

#define BANDCOUNT 4
#define CHANSPERBAND 8

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

int currentfreq = 0;
int currentband = 2;
int currentchan = 4;
int currentrssi = 0;
int scanindex = 0;
int chanindex = 0;
boolean rssilocked = false;
unsigned long locktime = 0;

const char *bandid = "DEAR"; // ImmersionRC (D), boscam E, boscam A, Raceband

const int chanfreq[] = { 
  5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880,   // FatShark / ImmersionRC
  5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945,   // Boscam E
  5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725,   // Boscam A
  5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917 }; // RaceBand

const int freqorder[] = {
  24, 41, 23, 22, 42, 21, 38, 43, 11, 37, 12, 36,
  44, 13, 35, 14, 34, 45, 15, 33, 16, 46, 32, 17,
  31, 18, 47, 25, 26, 48, 27, 28 };

void setup()
{
  Serial.begin(9600); 

  // channel select pins
  pinMode(CS1PIN, OUTPUT);
  pinMode(CS2PIN, OUTPUT);
  pinMode(CS3PIN, OUTPUT);

  // band select pins
  pinMode(BS1PIN, OUTPUT);
  pinMode(BS2PIN, OUTPUT);

  // lowest frequency
  setBand(2);
  setChannel(4);

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
  Serial.println("VergoRX 0.7");
  lcd.clear();
  lcd2.clear();
  lcd.writeDisplay();
  lcd2.writeDisplay();

  showText("VGO ");
  showText2("RX  ");
  lcd2.writeDigitAscii(2, '0', true);
  lcd2.writeDigitAscii(3, '7');
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
  Serial.print("band ");
  Serial.print(currentband);
  Serial.print(" (");
  Serial.print(bandid[currentband-1]);
  Serial.print(") ch ");
  Serial.print(currentchan);
  Serial.print(" (");
  Serial.print(chanfreq[currentchan-1+(currentband-1)*CHANSPERBAND]);
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
  // LOW = ON, connected to ground
  switch (channel) {
  case 1:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, LOW);
    break;
  case 2:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, HIGH);
    break;
  case 3:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, HIGH);
    digitalWrite(CS3PIN, LOW);
    break;
  case 4:
    digitalWrite(CS1PIN, LOW);
    digitalWrite(CS2PIN, HIGH);
    digitalWrite(CS3PIN, HIGH);
    break;
  case 5:
    digitalWrite(CS1PIN, HIGH);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, LOW);
    break;
  case 6:
    digitalWrite(CS1PIN, HIGH);
    digitalWrite(CS2PIN, LOW);
    digitalWrite(CS3PIN, HIGH);
    break;
  case 7:
    digitalWrite(CS1PIN, HIGH);
    digitalWrite(CS2PIN, HIGH);
    digitalWrite(CS3PIN, LOW);
    break;
  case 8:
    digitalWrite(CS1PIN, HIGH);
    digitalWrite(CS2PIN, HIGH);
    digitalWrite(CS3PIN, HIGH);
    break;
  default:
    break; 
  }
}

void setBand(int band)
{
  // LOW = ON, connected to ground
  switch (band) {
  case 1: // fatshark / immersionrc
    digitalWrite(BS1PIN, HIGH);
    digitalWrite(BS2PIN, HIGH);
    break;
  case 2: // boscam e
    digitalWrite(BS1PIN, HIGH);
    digitalWrite(BS2PIN, LOW);
    break;
  case 3: // boscam a
    digitalWrite(BS1PIN, LOW);
    digitalWrite(BS2PIN, HIGH);
    break;
  case 4: // raceband
    digitalWrite(BS1PIN, LOW);
    digitalWrite(BS2PIN, LOW);
    break;
  }
}

void changeChannel()
{
  char freq[5];

  currentfreq++;
  if (currentfreq >= BANDCOUNT*CHANSPERBAND-1) {
    currentfreq = 0; 
  }

  currentband = freqorder[currentfreq] / 10;
  currentchan = freqorder[currentfreq] - (currentband * 10);

  setBand(currentband);
  setChannel(currentchan);

  snprintf(freq, 5, "%4d", chanfreq[currentchan-1+(currentband-1)*CHANSPERBAND]);
  showText2(freq);

  delay(RSSISETTLETIME);
}

void showChanInfo()
{
  char t[5];

  snprintf(t, 5, "%c%d%02d", bandid[currentband-1], currentchan, currentrssi);
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


