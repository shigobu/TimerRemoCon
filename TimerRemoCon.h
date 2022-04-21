#ifndef TIMER_REMO_CON_H
#define TIMER_REMO_CON_H

#define EVERY_DAY 0b1111111
#define WEEK_DAY 0b0111110
#define WEEK_END 0b1000001
#define ONCE 0b0000000

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
  DeleteData,
  AlarmTest,
  LearnDetail,
  AlarmDetail,
  TimeSettingDetail,
  DeleteDataDetail,
  AlarmTestDetail,
};

struct AlarmSetting {
  uint8_t minute;  // 0-59
  uint8_t hour;    // 0-23
  uint8_t week;  // 0x01(Sun)-0x40(Sat)各ビットをセットする。NO_WEEKで一回のみ。
  uint8_t irIndex;  //使用する赤外線信号のインデックス
  bool isEnable;    //アラームが有効かどうか
  bool isSent;      //赤外線送信済みかどうか
};

// http://jsdiy.web.fc2.com/lcdclock/
// からダウンロードできるソースコードから引用。"日"を最初へ移動。
const uint8_t weekFont[][8] PROGMEM = {
    //日
    {0b11111, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b11111, 0b00000},
    //月
    {0b01111, 0b01001, 0b01111, 0b01001, 0b01111, 0b01001, 0b10001, 0b00000},
    //火
    {0b00100, 0b10101, 0b10101, 0b00100, 0b01010, 0b01010, 0b10001, 0b00000},
    //水
    {0b00100, 0b00101, 0b11110, 0b00110, 0b01101, 0b10100, 0b00100, 0b00000},
    //木
    {0b00100, 0b00100, 0b11111, 0b00100, 0b01110, 0b10101, 0b00100, 0b00000},
    //金
    {0b01110, 0b10001, 0b01110, 0b11111, 0b00100, 0b10101, 0b11111, 0b00000},
    //土
    {0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00100, 0b11111, 0b00000},
};

void PrintAlarmSetting(AlarmSetting& alarm, uint8_t col, uint8_t row,
                       bool printWeekNum = false);

#endif
