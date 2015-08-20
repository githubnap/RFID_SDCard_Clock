// Date, Time and Alarm functions using a DS3231 RTC connected via I2C and Wire lib

#include <Wire.h>
#include <SPI.h> // not used here, but needed to prevent a RTClib compile error
#include "RTClib.h"

RTC_DS3231 RTC;
String Write_in_text;

void setup () {

  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  DateTime now = RTC.now();
  RTC.setAlarm1Simple(23, 9);

  RTC.turnOnAlarm(1);

  if (RTC.checkAlarmEnabled(1)) {
    Serial.println("Alarm Enabled");
  }

}

void loop () {

  DateTime now = RTC.now();
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  
  Serial.println("-------------");
  
  Write_in_text =(String) now.day();
  Write_in_text += "/";
  Write_in_text += now.month();
  Write_in_text += "/";
  Write_in_text += now.year();
  Write_in_text += "     ";
  Write_in_text += now.hour();
  Write_in_text += ":";
  Write_in_text += now.minute();
  Write_in_text += ":";
  Write_in_text += now.second();
  Serial.println(Write_in_text);


  if (RTC.checkIfAlarm(1)) {
      Serial.println();
    Serial.println("Alarm Triggered");
  }

  Serial.println();
  Serial.print("Tempeature = ");
  Serial.print(RTC.getTemperature());
  Serial.println(" C");
  Serial.println();

  delay(1000);
}
