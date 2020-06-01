#include <SPI.h>
#include "DW1000Ranging.h"
#include <avr/eeprom.h>

// connection pins
const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 2; // irq pin
const uint8_t PIN_SS = SS; // spi select pin
const float DEF_RANGE = 2.0;
const uint16_t ANC1 = 11;
const uint16_t ANC2 = 12;
bool IS_DEBUG = true;

struct settings_t
{
  uint16_t anchor1;
  uint16_t anchor2;
  uint16_t antenna;
  float range;
  float pulse;
} settings;

void setup() {
  readSettings();

  Serial.begin(115200);

  //init the configuration
  DW1000.setAntennaDelay(settings.antenna);
  DW1000.setPulseFrequency(settings.pulse);
  DW1000.tune();
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  //Enable the filter to smooth the distance
  DW1000Ranging.useRangeFilter(true);

  //we start the module as a tag
  DW1000Ranging.startAsTag("0A:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_FAST_LOWPOWER, DW1000.CHANNEL_5, false);

  /*Serial.print(F("anchor1: ")); Serial.println(settings.anchor1);
    Serial.print(F("anchor2: ")); Serial.println(settings.anchor2);
    Serial.print(F("pulse: ")); Serial.println(settings.pulse);
    Serial.print(F("antenna: ")); Serial.println(settings.antenna);
    Serial.print(F("range: ")); Serial.println(settings.range);*/
}

void loop() {
  DW1000Ranging.loop();
  getSerialInput();
}

void newRange() {
  uint16_t namezzz = DW1000Ranging.getDistantDevice()->getShortAddress();
  if (namezzz == settings.anchor1 || namezzz == settings.anchor2) {
    float current_distance = DW1000Ranging.getDistantDevice()->getRange();
    if (current_distance <= settings.range && current_distance > 0) {
      if (IS_DEBUG) {
        Serial.print(F("from: ")); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
        Serial.print(F("\t Range: ")); Serial.print(DW1000Ranging.getDistantDevice()->getRange()); Serial.println(F(" m"));
      }
    }
    /*else {
      Serial.print(F("outlier from: ")); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
      Serial.print(F("\t Range: ")); Serial.print(DW1000Ranging.getDistantDevice()->getRange()); Serial.println(F(" m"));
      }*/
  }
}

void newDevice(DW1000Device* device) {
  if (IS_DEBUG) {
    Serial.print(F("ranging init; 1 device added ! -> "));
    Serial.println(device->getShortAddress(), HEX);
  }
}

void inactiveDevice(DW1000Device* device) {
  if (IS_DEBUG) {
    Serial.print(F("delete inactive device: "));
    Serial.println(device->getShortAddress(), HEX);
  }
}

void getSerialInput() {
  String inStr = "";
  // check for new setting
  while (Serial.available() > 0) {
    inStr = Serial.readString();
    inStr.toLowerCase();
    inStr.trim();
  }
  if (inStr != "") {
    if (inStr.startsWith("pulse")) {
      inStr.replace("pulse ", "");
      String out_config = F("Pulse Frequency:\t");
      if (inStr.toInt() > 0) {
        settings.pulse = inStr.toInt();
        DW1000.setPulseFrequency(inStr.toInt());
        DW1000.commitConfiguration();
        delay(1000);
      }
      Serial.print(out_config); Serial.println(DW1000.getPulseFrequency());
    }
    else if (inStr.startsWith("range")) {
      inStr.replace("range ", "");
      String out_config = F("Max Range:\t");
      if (inStr.toFloat() != 0) {
        settings.range = inStr.toFloat();
      }
      Serial.print(out_config); Serial.print(settings.range); Serial.println(" m");
    }
    else if (inStr.startsWith("anchor 1")) {
      inStr.replace("anchor 1 ", "");
      String out_config = F("Anchor 1:\t");
      if (inStr.toInt() != 0) {
        settings.anchor1 = inStr.toInt();
      }
      Serial.print(out_config); Serial.println(settings.anchor1);
    }
    else if (inStr.startsWith("anchor 2")) {
      inStr.replace("anchor 2 ", "");
      String out_config = F("Anchor 2:\t");
      if (inStr.toInt() != 0) {
        settings.anchor2 = inStr.toInt();
      }
      Serial.print(out_config); Serial.println(settings.anchor2);
    }
    else if (inStr.startsWith("antenna")) {
      inStr.replace("antenna ", "");
      String out_config = F("Antenna Delay:\t");
      if (inStr.toInt() > 0) {
        settings.antenna = inStr.toInt();
        DW1000.setAntennaDelay(inStr.toInt());
        DW1000.commitConfiguration();
        delay(1000);
      }
      Serial.print(out_config); Serial.println(DW1000.getAntennaDelay());
    }
    else if (inStr.startsWith("debug")) {
      inStr.replace("debug ", "");
      if (inStr.toInt() > 0) {
        IS_DEBUG = true;
        Serial.println("Debugging enabled");
        delay(1000);
      }
      else {
        IS_DEBUG = false;
        Serial.println("Debugging disabled");
      }
    }
    else if (inStr.equals("save config")) {
      eeprom_write_block((const void*)&settings, (void*)0, sizeof(settings));
      Serial.println(F("Config saved !!!"));
    }
  }
  inStr = "";
  // check for new setting
}

void readSettings() {
  eeprom_read_block((void*)&settings, (void*)0, sizeof(settings));

  if (isnan(settings.anchor1) || settings.anchor1 == 0) {
    settings.anchor1 = ANC1;
  }

  if (isnan(settings.anchor2) || settings.anchor2 == 0) {
    settings.anchor2 = ANC2;
  }

  if (isnan(settings.range) || settings.range == 0) {
    settings.range = DEF_RANGE;
  }

  if (isnan(settings.pulse) || settings.pulse == 0) {
    settings.pulse = 2.0;
  }

  if (isnan(settings.antenna) || settings.antenna == 0) {
    settings.antenna = 16384;
  }
}
