#ifndef TIMER_REMO_CON_H
#define TIMER_REMO_CON_H

enum buttonStatus {
  NUM0,
  NUM1,
  NUM2,
  NUM3,
  NUM4,
  NUM5,
  NUM6,
  NUM7,
  NUM8,
  NUM9,
  FUNC,
  ENTER,
  MODE,
  BC,
  NOT_PRESSED
};

enum ModeMessage {
  DateTime,
  Learn,
  Alarm,
  TimeSetting,
  LearnDetail,
  AlarmDetail,
  TimeSettingDetail,
};

//http://jsdiy.web.fc2.com/lcdclock/ からダウンロードできるソースコードから引用。"日"を最初へ移動。
const	uint8_t	weekFont[][8] PROGMEM =
{
  { //日
    0b11111,
    0b10001,
    0b10001,
    0b11111,
    0b10001,
    0b10001,
    0b11111,
    0b00000
  },
  { //月
    0b01111,
    0b01001,
    0b01111,
    0b01001,
    0b01111,
    0b01001,
    0b10001,
    0b00000
  },
  { //火
    0b00100,
    0b10101,
    0b10101,
    0b00100,
    0b01010,
    0b01010,
    0b10001,
    0b00000
  },
  { //水
    0b00100,
    0b00101,
    0b11110,
    0b00110,
    0b01101,
    0b10100,
    0b00100,
    0b00000
  },
  { //木
    0b00100,
    0b00100,
    0b11111,
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b00000
  },
  { //金
    0b01110,
    0b10001,
    0b01110,
    0b11111,
    0b00100,
    0b10101,
    0b11111,
    0b00000
  },
  { //土
    0b00100,
    0b00100,
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b11111,
    0b00000
  },
};


void SetModeMessage();
ModeMessage GetModeMessage();
void DispDateTime();
void DispBigTime();
buttonStatus GetButton();
buttonStatus WaitForButton();
void LearnMode();
void AlarmMode();
void TimeSettingMode();
char GetCharFromButton(buttonStatus button);
void SetLCDbacklight(bool isOn);
uint8_t GetRX8900WeekDayFromTimeHData(int8_t week);

#endif
