#include <SPI.h>
#include <DW1000Ranging.h>
#include <avr/eeprom.h>
#include <SoftwareSerial.h>

// connection pins
const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 2; // irq pin
const uint8_t PIN_SS = SS; // spi select pin
const float DEF_RANGE = 1.0;
const uint16_t DEF_TAG = 10;
float a_arr[10] = {DEF_RANGE, DEF_RANGE, DEF_RANGE, DEF_RANGE, DEF_RANGE, DEF_RANGE, DEF_RANGE, DEF_RANGE, DEF_RANGE, DEF_RANGE};
bool IS_DEBUG = true;

struct settings_t
{
  uint16_t tag;
  uint16_t antenna;
  float range;
  float pulse;
} settings;

SoftwareSerial serial1(6, 7); // RX, TX

void setup() {
  serial1.begin(9600);

  readSettings();
  Serial.begin(115200);

  //DW1000.setAntennaDelay(16484); //Default 16384
  DW1000.setAntennaDelay(settings.antenna);
  DW1000.setPulseFrequency(settings.pulse);
  DW1000.tune();
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachBlinkDevice(newBlink);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  //Enable the filter to smooth the distance
  DW1000Ranging.useRangeFilter(true);

  //we start the module as an anchor
  DW1000Ranging.startAsAnchor("0C:00:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY, DW1000.CHANNEL_5, false);

  /*Serial.print(F("tag: ")); Serial.println(settings.tag);
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
  if (namezzz == settings.tag) {
    float current_distance = DW1000Ranging.getDistantDevice()->getRange();
    if (current_distance <= settings.range && current_distance > 0) {
      shiftDistance(); a_arr[9] = current_distance ;
      float avg_distance = 0;
      for (int i = 0; i < 9 ; i++)
      {
        avg_distance += a_arr[i];
      }
      avg_distance = avg_distance / 10;
      if (IS_DEBUG) {
        Serial.print(F("from: ")); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
        Serial.print(F("\t Range: ")); Serial.print(DW1000Ranging.getDistantDevice()->getRange()); Serial.print(F(" m"));
        Serial.print(F("\t Avg Range: ")); Serial.print(avg_distance); Serial.println(" m");
      }

      serial1.println("#" + String(avg_distance));
      delay(1000);
    }
    /*else {
      Serial.print(F("outlier rom: ")); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
      Serial.print(F("\t Range: ")); Serial.print(DW1000Ranging.getDistantDevice()->getRange()); Serial.println(F(" m"));
      }*/
  }
}

void newBlink(DW1000Device* device) {
  if (IS_DEBUG) {
    Serial.print(F("blink; 1 device added ! -> "));
    Serial.println(device->getShortAddress(), HEX);
  }
}

void inactiveDevice(DW1000Device* device) {
  if (IS_DEBUG) {
    Serial.print(F("Delete inactive device : "));
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
        for (int i = 0; i < 10; i++)
        {
          a_arr[i] = settings.range;
        }
      }
      Serial.print(out_config); Serial.print(settings.range); Serial.println(" m");
    }
    else if (inStr.startsWith("tag")) {
      inStr.replace("tag ", "");
      String out_config = F("Tag:\t");
      if (inStr.toInt() != 0) {
        settings.tag = inStr.toInt();
      }
      Serial.print(out_config); Serial.println(settings.tag);
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

void shiftDistance()
{
  for (int i = 1; i < 10; i++)
  {
    a_arr[i - 1] = a_arr[i];
  }
}

void readSettings() {
  eeprom_read_block((void*)&settings, (void*)0, sizeof(settings));

  if (isnan(settings.tag) || settings.tag == 0) {
    settings.tag = DEF_TAG;
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
