/*
  https://smtengkapi.com/engineer-arduino-rtc
  から引用
*/

#include <Wire.h>
#include <Arduino.h>

#define RTC_C
#include "RX8900.h"
#undef RTC_C

void RX8900Class::begin() {
  struct dateTime dt = { 0, 0, 0, THU, 1, 10, 20 };
  begin(&dt);
};

void RX8900Class::begin(struct dateTime *dt) {
  delay(30);
  Wire.begin();
  uint8_t flgreg_init;
  getRegisters(FLAG_REG, 1, &flgreg_init);

  if (flgreg_init & 0x02) {  //VLFの確認(バックアップが有効でない)
    delay(2970);
    setDateTime(dt);
    uint8_t extreg = 0x08;
    setRegisters(EXTENSION_REG, 1, &extreg);
    uint8_t flgreg = 0x00;
    setRegisters(FLAG_REG, 1, &flgreg);
    uint8_t conreg = 0x41;
    setRegisters(CONTROL_REG, 1, &conreg);
    uint8_t backup = 0x00;
    setRegisters(BACKUP_FUNC_REG, 1, &backup);
  }
}

void RX8900Class::setRegisters(uint8_t address, int sz, uint8_t *data) {
  Wire.beginTransmission(RX8900A_ADRS);
  Wire.write(address);
  Wire.write(data, sz);
  Wire.endTransmission();
}

void RX8900Class::getRegisters(uint8_t address, int sz, uint8_t *data) {
  Wire.beginTransmission(RX8900A_ADRS);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(RX8900A_ADRS, sz);

  for (int i = 0; i < sz; i++) {
    data[i] = Wire.read();
  }
}

void RX8900Class::setDateTime(struct dateTime *dt) {
  uint8_t data[7];

  data[0] = decimalToBCD(dt->second);
  data[1] = decimalToBCD(dt->minute);
  data[2] = decimalToBCD(dt->hour);
  data[3] = dt->week;  //weekのみ各ビットを立てる
  data[4] = decimalToBCD(dt->day);
  data[5] = decimalToBCD(dt->month);
  data[6] = decimalToBCD(dt->year);

  setRegisters(SEC_REG, 7, data);
}

void RX8900Class::getDateTime(struct dateTime *dt) {
  uint8_t data[7];

  getRegisters(SEC_REG, 7, data);
  dt->second = BCDToDecimal(data[0] & 0x7f);
  dt->minute = BCDToDecimal(data[1] & 0x7f);
  dt->hour = BCDToDecimal(data[2] & 0x3f);
  dt->week = data[3];
  dt->day = BCDToDecimal(data[4] & 0x3f);
  dt->month = BCDToDecimal(data[5] & 0x1f);
  dt->year = BCDToDecimal(data[6]);
}

void RX8900Class::getTemp(uint8_t *temp) {

  getRegisters(TEMP_REG, 1, temp);
}

inline int RX8900Class::decimalToBCD(int decimal) {
  return (((decimal / 10) << 4) | (decimal % 10));
}

inline int RX8900Class::BCDToDecimal(int bcd) {
  return ((bcd >> 4) * 10 + (bcd & 0x0f));
}
