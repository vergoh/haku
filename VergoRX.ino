#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define DEBUG 1

#define LCDBRIGHTNESS 2

#define BS1PIN 8
#define BS2PIN 9

// note that pin order in receiver is inverse
// compared to fatshark / immersionrc transmitters
#define CS1PIN 10
#define CS2PIN 11
#define CS3PIN 12

#define BANDCOUNT    4
#define CHANSPERBAND 8

#define RSSIPIN        A6
#define RSSIMAX        560
#define RSSIMINLOCK    50
#define RSSISETTLETIME 200

#define RSSIKEEPLOCK       5000
#define RSSICHECKINTERVAL  500

#define MAJORVERSION '0'
#define MINORVERSION '9'

Adafruit_AlphaNum4 lcd = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 lcd2 = Adafruit_AlphaNum4();

byte currentfreq = 0;
byte currentband = 0;
byte currentchan = 0;
int currentrssi = 0;
byte scanindex = 0;
int rssinoisefloor = RSSIMAX;
boolean rssilocked = false;
unsigned long locktime = 0;
unsigned long lastrssicheck = 0;

const byte enabledbands[] = { 
  1, 2, 3 };

const char *bandid = "DEAR"; // ImmersionRC (D), Boscam E, Boscam A, RaceBand

const int chanfreq[] = { 
  5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880,   // FatShark / ImmersionRC
  5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945,   // Boscam E
  5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725,   // Boscam A
  5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917 }; // RaceBand

// 18 is intentionally missing before 47 as the 5880 frequency is
// a duplicate and not officially in the ImmersionRC band list
const byte freqorder[] = {
  24, 41, 23, 22, 42, 21, 38, 43, 11, 37, 12, 36,
  44, 13, 35, 14, 34, 45, 15, 33, 16, 46, 32, 17,
  31, 47, 25, 26, 48, 27, 28 };

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
  setRSSINoiseFloor();
  showSetup();
  delay(4000);
}

void loop()
{
  if ( (rssilocked || keepLock()) && (millis()-lastrssicheck < RSSICHECKINTERVAL) ) {
    return;
  }

  checkRssiLock();
  if ( !rssilocked && !keepLock() ) {
    scanAnimation();
    changeChannel();
  }
  else {
    showChanInfo();
  }
}

void showInfo()
{
  Serial.print("VergoRX ");
  Serial.print(MAJORVERSION);
  Serial.print(".");
  Serial.println(MINORVERSION);
  lcd.clear();
  lcd2.clear();
  lcd.writeDisplay();
  lcd2.writeDisplay();

  showText("VGO ");
  showText2("RX  ");
  lcd2.writeDigitAscii(2, MAJORVERSION, true);
  lcd2.writeDigitAscii(3, MINORVERSION);
  lcd2.writeDisplay();
}

void showSetup()
{
  byte chancount;
  char buffer[5];

  chancount = getChanCount();
  snprintf(buffer, 5, "%02dch", chancount);
  showText(buffer);

  snprintf(buffer, 5, "____");;
  for (byte i = 0; i < 4; i++) {
    if (isBandEnabled(i+1)) {
      buffer[i] = bandid[i];
    }
  }
  showText2(buffer);

  if (!DEBUG) {
    return; 
  }

  Serial.print(chancount);
  Serial.print(" channels, active bands: ");
  Serial.println(buffer);
}

void showText(char *buffer)
{
  if (strlen(buffer) != 4) {
    return;
  }

  for (byte i = 0; i < 4; i++) {
    lcd.writeDigitAscii(i, buffer[i]);
  }
  lcd.writeDisplay();
}

void showText2(char *buffer)
{
  if (strlen(buffer) != 4) {
    return;
  }

  for (byte i = 0; i < 4; i++) {
    lcd2.writeDigitAscii(i, buffer[i]);
  }
  lcd2.writeDisplay();
}

void scanAnimation()
{
  char buffer[5];
  char spinner[] = "-\\|/";

  snprintf(buffer, 5, "%c %02d", spinner[scanindex], currentrssi);
  for (byte i = 0; i < 4; i++) {
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
  if (!DEBUG) {
    return;
  }
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

int readRSSI(boolean raw = false)
{
  int rssi = 0;
  int rssi_scaled = 0;

  rssi = analogRead(RSSIPIN);

  if (raw) {
    return rssi;
  }

  rssi = rssi  - rssinoisefloor;
  if (rssi < 0) {
    rssi = 0;    
  }
  rssi_scaled = rssi / (float)(RSSIMAX - rssinoisefloor) * 100;
  if (rssi_scaled >= 100) {
    rssi_scaled = 99;
  }
  showRSSIInfo(rssi, rssi_scaled);
  return rssi_scaled;
}

void channelOptimizer()
{
  int rssi;
  byte prevfreq;

  showText("TUNE");
  debugprintln("optimizing...");
  // nothing past highest frequency
  if (currentfreq == sizeof(freqorder)/sizeof(int)-1) {
    debugprintln("optimizer: at end of frequency list");
    return; 
  }

  prevfreq = currentfreq;
  changeChannel();
  rssi = readRSSI();
  if (rssi <= currentrssi) {
    debugprintln("optimizer: previous channel was better");
    currentfreq = prevfreq - 1;
    changeChannel();
    return;
  } 
  else {
    currentrssi = rssi;
  }
  channelOptimizer();
}

void checkRssiLock()
{
  currentrssi = readRSSI();
  if (currentrssi >= RSSIMINLOCK) { 
    if (!rssilocked && !keepLock()) {
      channelOptimizer();
    }
    rssilocked = true;
    locktime = millis();
  } 
  else {
    rssilocked = false; 
  }
  lastrssicheck = millis();
}

boolean keepLock()
{
  if (locktime == 0) {
    return false;
  }
  if (millis()-locktime > RSSIKEEPLOCK) {
    return false;
  }
  return true;
}

byte getChanCount()
{
  byte count = 0;
  for (byte i = 0; i < sizeof(freqorder)/sizeof(byte); i++) {
    if (isBandEnabled(freqorder[i] / 10)) {
      count++; 
    }
  }
  return count;
}

boolean isBandEnabled(byte band)
{
  for (byte i = 0; i < sizeof(enabledbands)/sizeof(byte); i++) {
    if (enabledbands[i] == band) {
      return true;         
    }
  } 
  return false;
}

void changeChannel()
{
  char freq[5];
  byte band;

  currentfreq++;
  if (currentfreq >= sizeof(freqorder)/sizeof(byte)-1) {
    currentfreq = 0; 
  } 

  band = freqorder[currentfreq] / 10;

  if (!isBandEnabled(band)) {
    changeChannel();
    return; 
  }

  setBand(band);
  setChannel(freqorder[currentfreq] - (currentband * 10));

  snprintf(freq, 5, "%4d", chanfreq[currentchan-1+(currentband-1)*CHANSPERBAND]);
  showText2(freq);

  delay(RSSISETTLETIME);
}

void setRSSINoiseFloor()
{
  int rssi = 0;

  debugprintln("scanning rssi noise floor..."); 

  for (byte i = 0; i < sizeof(freqorder)/sizeof(byte); i = i + 2) {
    setBand(freqorder[i] / 10);
    setChannel(freqorder[i] - (currentband * 10));
    delay(200);
    rssi = readRSSI(true);
    if (rssi < rssinoisefloor) {
      rssinoisefloor = rssi;
    }
  }

  if (DEBUG) {
    Serial.print("rssi noise floor: ");
    Serial.println(rssinoisefloor);
  }

  setBand(freqorder[0] / 10);
  setChannel(freqorder[0] - (currentband * 10));
  delay(RSSISETTLETIME);
}

void showChanInfo()
{
  char t[5];

  snprintf(t, 5, "%c%d%02d", bandid[currentband-1], currentchan, currentrssi);
  for (byte i = 0; i < 4; i++) {
    if (i == 1) {
      lcd.writeDigitAscii(i, t[i], true);
    } 
    else{
      lcd.writeDigitAscii(i, t[i]);
    }
  }
  lcd.writeDisplay();
}

void debugprintln(char *string)
{
  if (DEBUG) {
    Serial.println(string); 
  }
}

void setChannel(byte channel)
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
  currentchan = channel;
}

void setBand(byte band)
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
  default:
    break;
  }
  currentband = band;
}





