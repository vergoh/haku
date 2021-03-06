#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define DEBUG 0

#define OUTDOORBRIGHTNESS 200
#define LCDINDOORBRIGHTNESS 2
#define LCDOUTDOORBRIGHTNESS 15

#define BRIGHTNESSPIN  A1

#define HOLDBPIN 6
#define NEXTBPIN 7

#define BS1PIN 8
#define BS2PIN 9

// note that pin order in receiver is inverse
// compared to fatshark / immersionrc transmitters
#define CS1PIN 10
#define CS2PIN 11
#define CS3PIN 12

#define RSSIPIN        A6
#define RSSIMINLOCK    50
#define RSSIMINRANGE 200
#define RSSISETTLETIME 200

#define RSSIKEEPLOCK       5000
#define RSSICHECKINTERVAL  300

#define BANDCOUNT    4
#define CHANSPERBAND 8

#define BUTTONPRESSTIME 50
#define BUTTONLONGPRESSTIME 1000

#define MAJORVERSION '1'
#define MINORVERSION '4'

Adafruit_AlphaNum4 lcd = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 lcd2 = Adafruit_AlphaNum4();

int rssimin = 1024;
int rssimax = 0;

byte currentfreq = 0;
byte currentband = 0;
byte currentchan = 0;
int currentrssi = 0;
byte scanindex = 0;
boolean rssilocked = false;
unsigned long locktime = 0;
unsigned long lastrssicheck = 0;
unsigned long channelchange = 0;

boolean holdmode = false;

boolean hold = false;
boolean holdlong = false;
boolean next = false;

boolean holdbstate = false;
boolean nextbstate = false;
unsigned long holdbhigh = 0;
unsigned long nextbhigh = 0;

boolean enabledbands[] = {
  true, true, true, true };

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

  // buttons
  pinMode(HOLDBPIN, INPUT_PULLUP);
  pinMode(NEXTBPIN, INPUT_PULLUP);

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
  lcd.writeDisplay();

  // init secondary display
  lcd2.begin(0x71);
  lcd2.clear();
  lcd2.writeDisplay();

  setBrightness();
  showInfo();
  initRSSIRange();
  showSetup();
  delay(3000);
}

void loop()
{
  checkButtons();
  reactToButtons();

  if (millis()-channelchange < RSSISETTLETIME) {
    return;
  }

  if ( (rssilocked || keepLock() || holdmode) && (millis()-lastrssicheck < RSSICHECKINTERVAL) ) {
    return;
  }

  checkRssiLock();
  if ( !rssilocked && !keepLock() && !holdmode ) {
    scanAnimation();
    changeChannel();
  }
  else {
    showChanInfo();
  }
}

void showInfo()
{
  Serial.print("Haku v");
  Serial.print(MAJORVERSION);
  Serial.print(".");
  Serial.println(MINORVERSION);
  lcd.clear();
  lcd2.clear();
  lcd.writeDisplay();
  lcd2.writeDisplay();

  showText("HAKU");
  lcd2.writeDigitAscii(1, MAJORVERSION, true);
  lcd2.writeDigitAscii(2, MINORVERSION);
  lcd2.writeDisplay();
}

void showSetup()
{
  byte chancount;
  char buffer[5];

  chancount = getChanCount();
  snprintf(buffer, 5, "%2dch", chancount);
  showText(buffer);

  snprintf(buffer, 5, "____");

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

void setBrightness()
{
  byte brightness = LCDINDOORBRIGHTNESS;
  int b = 0;

  b = 1023 - analogRead(BRIGHTNESSPIN);

  if (b >= OUTDOORBRIGHTNESS) {
    brightness = LCDOUTDOORBRIGHTNESS;
  }

  lcd.setBrightness(brightness);
  lcd.writeDisplay();
  lcd2.setBrightness(brightness);
  lcd2.writeDisplay();

  if (!DEBUG) {
    return;
  }

  Serial.print("brightness: ");
  Serial.print(b);
  Serial.print(" -> LCD: ");
  Serial.println(brightness);
}

void checkButtons()
{
  // "hold" button, pin low when pressed
  if (digitalRead(HOLDBPIN) == LOW) {
    if (holdbstate && (millis()-holdbhigh > BUTTONPRESSTIME)) {
      hold = true;
      debugprintln("hold button pressed");
      while(true) {
        if (digitalRead(HOLDBPIN) == HIGH) {
          if (millis()-holdbhigh > BUTTONLONGPRESSTIME) {
            hold = false;
            holdlong = true;
          }
          break;
        }
        delay(20);
      }
    } 
    else {
      holdbstate = true;
    }
  } 
  else {
    holdbstate = false;
    holdbhigh = millis();
  }

  // "next" button, pin low when pressed
  if (digitalRead(NEXTBPIN) == LOW) {
    if (nextbstate && (millis()-nextbhigh > BUTTONPRESSTIME)) {
      next = true;
      debugprintln("next button pressed");
      while(true) {
        if (digitalRead(NEXTBPIN) == HIGH) {
          break; 
        }
        delay(20);
      }
    } 
    else {
      nextbstate = true;
    }
  } 
  else {
    nextbstate = false;
    nextbhigh = millis();
  }
}

void reactToButtons()
{
  if (holdlong) {
    holdlong = false;
    menu();
  }

  if (hold) {
    hold = false;
    holdmode = !holdmode;
    if (holdmode) {
      debugprintln("hold enabled");
    }
    else {
      debugprintln("hold disabled");
    }
  }

  if (next) {
    next = false;
    locktime = 0;
    lastrssicheck = 0;
    changeChannel();
  }
}

void menu()
{
  unsigned long showtime = 0;
  boolean shown = false;
  byte index = 0;
  char buffer[5];

  showText2("MENU");

  while (true) {
    checkButtons();

    if (holdlong) {
      holdlong = false;
      if (getChanCount() > 0) {
        break;
      }
    }
    if (hold) {
      hold = false;
      enabledbands[index] = !enabledbands[index];
    }
    if (next) {
      next = false;
      index++;
      if (index >= 4) {
        index = 0;
      }
    }

    if (millis()-showtime > 500) {
      shown = !shown;
      snprintf(buffer, 5, "____");
      for (byte i = 0; i < 4; i++) {
        if (i == index && !shown) {
          buffer[i] = ' ';
        }
        else if (isBandEnabled(i+1)) {
          buffer[i] = bandid[i];
        }
      }
      showText(buffer);
      showtime = millis();
    }
  }

  // reset status
  rssimin = 1024;
  rssimax = 0;
  currentfreq = 0;
  currentband = 0;
  currentchan = 0;
  currentrssi = 0;
  scanindex = 0;
  rssilocked = false;
  locktime = 0;
  lastrssicheck = 0;
  channelchange = 0;
  holdmode = false;

  // recalibrate
  showSetup();
  initRSSIRange();
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

  if (rssimax-rssimin >= RSSIMINRANGE) {
    snprintf(buffer, 5, "%c %02d", spinner[scanindex], getScaledRSSI(currentrssi));
  } 
  else {
    snprintf(buffer, 5, "%c%03d", spinner[scanindex], currentrssi);
  }
  for (byte i = 0; i < 4; i++) {
    lcd.writeDigitAscii(i, buffer[i]);
  }
  lcd.writeDisplay();
  scanindex++;
  if (scanindex >= 4) {
    scanindex = 0; 
  }
}

int getScaledRSSI(int rawrssi)
{
  int rssi = 0;
  int rssi_scaled = 0;

  rssi = rawrssi  - rssimin;
  if (rssi < 0) {
    rssi = 0;    
  }
  rssi_scaled = rssi / (float)(rssimax - rssimin) * 100;
  if (rssi_scaled >= 100) {
    rssi_scaled = 99;
  }
  return rssi_scaled;
}

void showRSSIInfo(int rssi)
{
  int rssi_scaled;
  char buffer[5];
  if (!DEBUG) {
    return;
  }
  if (rssilocked || holdmode) {
    snprintf(buffer, 5, "%4d", rssi);
    showText2(buffer);
  }
  rssi_scaled = getScaledRSSI(rssi);
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

int readRSSI(boolean noinfo = false)
{
  int rawrssi = 0;

  rawrssi = analogRead(RSSIPIN);

  if (rawrssi < rssimin) {
    rssimin = rawrssi;
  }
  if (rawrssi > rssimax) {
    rssimax = rawrssi; 
  }

  if (noinfo) {
    return rawrssi;
  }

  showRSSIInfo(rawrssi);
  return rawrssi;
}

void channelOptimizer()
{
  int rssi;
  byte prevfreq;

  if (holdmode) {
    return; 
  }

  showText("TUNE");
  debugprintln("optimizing...");
  // nothing past highest frequency
  if (currentfreq == sizeof(freqorder)/sizeof(byte)-1) {
    debugprintln("optimizer: at end of frequency list");
    return; 
  }

  prevfreq = currentfreq;
  changeChannel();
  delay(RSSISETTLETIME);
  rssi = readRSSI();
  if (getScaledRSSI(rssi) <= getScaledRSSI(currentrssi)) {
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
  if ( (getScaledRSSI(currentrssi) >= RSSIMINLOCK) && (rssimax-rssimin >= RSSIMINRANGE) ) { 
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
  return enabledbands[band-1];
}

void changeChannel()
{
  char freq[5];
  byte band;

  currentfreq++;
  if (currentfreq >= sizeof(freqorder)/sizeof(byte)) {
    currentfreq = 0; 
  } 

  band = freqorder[currentfreq] / 10;

  if (!isBandEnabled(band)) {
    changeChannel();
    return; 
  }

  setBand(band);
  setChannel(freqorder[currentfreq] - (currentband * 10));
  channelchange = millis();

  snprintf(freq, 5, "%4d", chanfreq[currentchan-1+(currentband-1)*CHANSPERBAND]);
  showText2(freq);
}

void initRSSIRange()
{
  int rssi = 0;

  debugprintln("rssi range init..."); 

  for (byte i = 0; i < sizeof(freqorder)/sizeof(byte); i = i + 2) {
    setBand(freqorder[i] / 10);
    setChannel(freqorder[i] - (currentband * 10));
    delay(RSSISETTLETIME);
    readRSSI(true);
  }

  if (DEBUG) {
    Serial.print("initial rssi range: ");
    Serial.print(rssimin);
    Serial.print(" - ");
    Serial.println(rssimax);
  }

  setBand(freqorder[0] / 10);
  setChannel(freqorder[0] - (currentband * 10));
  channelchange = millis();
}

void showChanInfo()
{
  char t[5];

  snprintf(t, 5, "%c%d%02d", bandid[currentband-1], currentchan, getScaledRSSI(currentrssi));
  for (byte i = 0; i < 4; i++) {
    if (i == 1 || (holdmode && i == 0)) {
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

