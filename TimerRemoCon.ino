#include <EEPROM.h>
#include <KanaLiquidCrystal.h>  // この#includeで、KanaLiquidCrystalライブラリを呼び出します。
#include <LiquidCrystal.h>  // LiquidCrystalライブラリも間接的に使うので、この#includeも必要です
#include <Narcoleptic.h>
#include <ResKeypad.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <time.h>

#define NO_LED_FEEDBACK_CODE
#define SEND_PWM_BY_TIMER
//#define IR_SEND_PIN 9     IRTimer.hpp
//を編集して送信PINを変更している。SEND_PWM_BY_TIMER
//使用時はこうする必要がある。
#include <IRremote.hpp>

#include "RX8900.h"
#include "TimerRemoCon.h"

const int LCDBacklightPin = 17;

const int AIN2 = A0;
const int AIN1 = A1;
const int KeyNum = 7;
PROGMEM const int threshold[KeyNum] = {
    // 次の数列は、しなぷすのハード製作記の回路設計サービスで計算して得られたもの
    42, 165, 347, 511, 641, 807, 965};
ResKeypad keypad2(AIN2, KeyNum, threshold);
ResKeypad keypad1(AIN1, KeyNum, threshold);

KanaLiquidCrystal lcd(8, 10, 11, 12, 13, 16);
//フォントの登録用
LiquidCrystal lcdNoKana(8, 10, 11, 12, 13, 16);

dateTime tim;
tm timeHstruct;
ModeMessage message = ModeMessage::DateTime;

unsigned long prevButtonMillis = 0;
unsigned int LCDbacklightOnMillis = 10000;
unsigned long prevGetTime = 0;
const int wakeupINT = 1;
volatile bool isSleeping = false;

const int IR_RECEIVE_PIN = 2;
// irData配列の各要素に有効な値が入っているかどうかをフラグで表している。下桁から順に格納されている。
int16_t irDataAvailables = 0;
const int dataMaxNum = 10;
IRData irData[dataMaxNum];
AlarmSetting alarmSetting[dataMaxNum];

void setup() {
  pinMode(LCDBacklightPin, OUTPUT);
  pinMode(AIN1, INPUT);
  pinMode(AIN2, INPUT);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcdNoKana.begin(16, 2);
  initWeekFont();

  SetLCDbacklight(true);
  lcd.clear();
  lcd.kanaOn();

  lcd.print(F("ｼｮｷｶﾁｭｳ"));
  RX8900.begin();
  IrReceiver.begin(IR_RECEIVE_PIN, false);
  IrReceiver.stop();
  IrSender.begin(false);
  lcd.setCursor(0, 0);
  lcd.print(F("ﾀｲﾏｰﾘﾓｺﾝ V0.1"));
  delay(1000);

  Narcoleptic.disableSerial();
  Narcoleptic.disableSPI();

  //データ読み込み
  LoadFromEEPROM();

  //スリープ復帰する割り込み
  attachInterrupt(wakeupINT, WakeupFunction, FALLING);
}

void loop() {
  RX8900.getDateTime(&tim);

  // アラーム処理
  AlarmProcessing();

  lcd.home();
  //メッセージの設定
  SetModeMessage();
  //メッセージ処理
  switch (GetModeMessage()) {
    case ModeMessage::DateTime:
      DispDateTime();
      break;
    case ModeMessage::Learn:
      lcd.print(F("ｶﾞｸｼｭｳ ﾓｰﾄﾞ     "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    case ModeMessage::LearnDetail:
      LearnMode();
      message = ModeMessage::DateTime;
      break;
    case ModeMessage::Alarm:
      lcd.print(F("ｱﾗｰﾑ ﾓｰﾄﾞ       "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    case ModeMessage::AlarmDetail:
      AlarmMode();
      message = ModeMessage::DateTime;
      break;
    case ModeMessage::TimeSetting:
      lcd.print(F("ｼﾞｶﾝ ｾｯﾃｲ       "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    case ModeMessage::TimeSettingDetail:
      TimeSettingMode();
      message = ModeMessage::DateTime;
      break;
    case ModeMessage::DeleteData:
      lcd.print(F("ﾃﾞｰﾀ ｻｸｼﾞｮ       "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    case ModeMessage::DeleteDataDetail:
      DeleteDataMode();
      message = ModeMessage::DateTime;
      break;
    case ModeMessage::AlarmTest:
      lcd.print(F("IR ﾃｽﾄ          "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    case ModeMessage::AlarmTestDetail:
      IrSendTest();
      message = ModeMessage::DateTime;
      break;
    default:
      break;
  }

  //スリープ
  switch (GetModeMessage()) {
    case ModeMessage::DateTime:
      if (millis() - prevButtonMillis > LCDbacklightOnMillis) {
        isSleeping = true;
        SetLCDbacklight(false);
        //バックライト消灯と同時に秒数を消すためにここでも時間表示
        DispDateTime();
        Narcoleptic.sleepAdv(WDTO_1S, SLEEP_MODE_PWR_DOWN, _BV(INT1));
      }
      break;
    case ModeMessage::Learn:
    case ModeMessage::Alarm:
    case ModeMessage::TimeSetting:
    case ModeMessage::DeleteData:
    case ModeMessage::AlarmTest:
    case ModeMessage::LearnDetail:
    case ModeMessage::AlarmDetail:
    case ModeMessage::TimeSettingDetail:
    case ModeMessage::DeleteDataDetail:
    case ModeMessage::AlarmTestDetail:
      prevButtonMillis = millis();
  }
}

//スリープから復帰した時のメソッド
void WakeupFunction() {
  isSleeping = false;
  SetLCDbacklight(true);
  prevButtonMillis = millis();
}

//アラーム処理
void AlarmProcessing() {
  for (size_t alarmIndex = 0; alarmIndex < dataMaxNum; alarmIndex++) {
    if (alarmSetting[alarmIndex].hour == tim.hour &&
        alarmSetting[alarmIndex].minute == tim.minute) {
      if (!alarmSetting[alarmIndex].isEnable) {
        continue;
      }

      if (alarmSetting[alarmIndex].isSent) {
        continue;
      }

      if ((alarmSetting[alarmIndex].week & tim.week) > 0) {
        //送信
        if (IsIrDataAvailable(alarmSetting[alarmIndex].irIndex)) {
          IrSender.write(&irData[alarmSetting[alarmIndex].irIndex]);
        }
        alarmSetting[alarmIndex].isSent = true;
      } else if (alarmSetting[alarmIndex].week == 0) {
        //送信
        if (IsIrDataAvailable(alarmSetting[alarmIndex].irIndex)) {
          IrSender.write(&irData[alarmSetting[alarmIndex].irIndex]);
        }
        alarmSetting[alarmIndex].isEnable = false;
      } else {
        //なにもしない
      }
      alarmSetting[alarmIndex].isSent = true;
    } else {
      alarmSetting[alarmIndex].isSent = false;
    }
  }
}

//フォントの登録
void initWeekFont() {
  uint8_t bb[8] = {0};
  for (int nb = 0; nb < 7; nb++) {
    for (int bc = 0; bc < 8; bc++) bb[bc] = pgm_read_byte(&weekFont[nb][bc]);
    //カナ対応版のライブラリだと何故か正常に登録できなかったので、通常版を使用。kanaOff()を使用してもダメだった。
    lcdNoKana.createChar(nb, bb);
  }
}

//モードボタンの押下状態を確認して、モードの切り替えを行います。
void SetModeMessage() {
  buttonStatus button = GetButton();
  if (button == buttonStatus::MODE) {
    switch (message) {
      case ModeMessage::DateTime:
        message = ModeMessage::Learn;
        break;
      case ModeMessage::Learn:
        message = ModeMessage::Alarm;
        break;
      case ModeMessage::Alarm:
        message = ModeMessage::TimeSetting;
        break;
      case ModeMessage::TimeSetting:
        message = ModeMessage::DeleteData;
        break;
      case ModeMessage::DeleteData:
        message = ModeMessage::AlarmTest;
        break;
      case ModeMessage::AlarmTest:
        message = ModeMessage::DateTime;
        break;
      default:
        break;
    }
  }

  if (button == buttonStatus::ENTER) {
    switch (message) {
      case ModeMessage::Learn:
        message = ModeMessage::LearnDetail;
        break;
      case ModeMessage::Alarm:
        message = ModeMessage::AlarmDetail;
        break;
      case ModeMessage::TimeSetting:
        message = ModeMessage::TimeSettingDetail;
        break;
      case ModeMessage::DeleteData:
        message = ModeMessage::DeleteDataDetail;
        break;
      case ModeMessage::AlarmTest:
        message = ModeMessage::AlarmTestDetail;
        break;
      default:
        break;
    }
  }

  if (button == buttonStatus::BC) {
    switch (message) {
      case ModeMessage::Learn:
      case ModeMessage::Alarm:
      case ModeMessage::TimeSetting:
      case ModeMessage::DeleteData:
      case ModeMessage::AlarmTest:
        message = ModeMessage::DateTime;
        break;
      default:
        break;
    }
  }
}

//モードメッセージを取得します。
ModeMessage GetModeMessage() { return message; }

// LCDに時間を表示します。
void DispDateTime() {
  //日付
  lcd.setCursor(0, 0);
  lcd.print(20);
  if (tim.year < 10) {
    lcd.print('0');
  }
  lcd.print(tim.year);
  lcd.print("/");

  if (tim.month < 10) {
    lcd.print('0');
  }
  lcd.print(tim.month);
  lcd.print("/");

  if (tim.day < 10) {
    lcd.print('0');
  }
  lcd.print(tim.day);

  PrintWeekDayToLcd();

  //時間
  lcd.setCursor(0, 1);
  if (tim.hour < 10) {
    lcd.print('0');
  }
  lcd.print(tim.hour);
  lcd.print(":");

  if (tim.minute < 10) {
    lcd.print('0');
  }
  lcd.print(tim.minute);

  if (isSleeping) {
    lcd.print(F("   "));
  } else {
    lcd.print(":");
    if (tim.second < 10) {
      lcd.print('0');
    }
    lcd.print(tim.second);
  }

  lcd.print(F("        "));
}

//ボタンの押下状態を取得します。
buttonStatus GetButton() {
  buttonStatus button = buttonStatus::NOT_PRESSED;

  if (isSleeping) {
    return button;
  }

  signed char key;
  key = keypad1.GetKey();
  switch (key) {
    case 0:
      button = buttonStatus::FUNC;
      break;
    case 1:
      button = buttonStatus::NUM0;
      break;
    case 2:
      button = buttonStatus::NUM1;
      break;
    case 3:
      button = buttonStatus::NUM2;
      break;
    case 4:
      button = buttonStatus::NUM3;
      break;
    case 5:
      button = buttonStatus::NUM4;
      break;
    case 6:
      button = buttonStatus::ENTER;
      break;
  }

  key = keypad2.GetKey();
  switch (key) {
    case 0:
      button = buttonStatus::MODE;
      break;
    case 1:
      button = buttonStatus::NUM5;
      break;
    case 2:
      button = buttonStatus::NUM6;
      break;
    case 3:
      button = buttonStatus::NUM7;
      break;
    case 4:
      button = buttonStatus::NUM8;
      break;
    case 5:
      button = buttonStatus::NUM9;
      break;
    case 6:
      button = buttonStatus::BC;
      break;
  }
  return button;
}

//ボタン入力を待ちます。
buttonStatus WaitForButton() {
  buttonStatus button = GetButton();
  while (button == buttonStatus::NOT_PRESSED) {
    button = GetButton();
  }
  return button;
}

//学習モード本体
void LearnMode() {
  lcd.setCursor(0, 0);
  lcd.print(F("ﾄｳﾛｸﾊﾞﾝｺﾞｳｼﾃｲ:0 "));
  lcd.cursor();
  lcd.blink();

  int8_t alarmDataIndex = 0;
  if (IsIrDataAvailable(alarmDataIndex)) {
    lcd.setCursor(0, 1);
    lcd.print(F("ﾃﾞｰﾀ ｱﾘ         "));
  } else {
    lcd.setCursor(0, 1);
    lcd.print(F("ﾃﾞｰﾀ ﾅｼ         "));
  }

numInput:
  lcd.setCursor(14, 0);
  buttonStatus button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    alarmDataIndex = button;
    if (IsIrDataAvailable(alarmDataIndex)) {
      lcd.setCursor(0, 1);
      lcd.print(F("ﾃﾞｰﾀ ｱﾘ         "));
    } else {
      lcd.setCursor(0, 1);
      lcd.print(F("ﾃﾞｰﾀ ﾅｼ         "));
    }
    goto numInput;
  } else if (button == buttonStatus::ENTER) {
    //次へ進む。
  } else if (button == buttonStatus::BC) {
    //キャンセル・終了。
    goto finally;
  } else {
    //数字とEnterとBC以外は無視して、入力へ戻る。
    goto numInput;
  }

  lcd.noCursor();
  lcd.noBlink();
  lcd.clear();
  lcd.print(alarmDataIndex);
  lcd.print(F(" ﾊﾞﾝﾆﾄｳﾛｸｼﾏｽ  "));

  IrReceiver.start(80000);
  //赤外線信号受信待機
  while (!IrReceiver.available()) {
    if (GetButton() == buttonStatus::BC) {
      return;
    };
  }

  IrReceiver.decode();
  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    lcd.setCursor(0, 1);
    lcd.print(F("ｴﾗｰ ﾐﾀｲｵｳｼﾝｺﾞｳ  "));
    goto delayfinally;
  } else {
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      lcd.setCursor(0, 1);
      lcd.print(F("ｴﾗｰ ﾐﾀｲｵｳｼﾝｺﾞｳ  "));
      goto delayfinally;
    }
  }

  if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
    irData[alarmDataIndex] = IrReceiver.decodedIRData;
    irDataAvailables |= 1 << alarmDataIndex;
    // eepromへ保存
    SaveToEEPROM();
    lcd.setCursor(0, 1);
    lcd.print(F("ﾄｳﾛｸｶﾝﾘｮｳ       "));
  }

delayfinally:
  IrReceiver.resume();
  delay(3000);

finally:
  IrReceiver.stop();
  lcd.noCursor();
  lcd.noBlink();
}

// EEPROMへ保存します
void SaveToEEPROM() {
  int address = 0;
  EEPROM.put(address, irDataAvailables);
  address += sizeof(irDataAvailables);

  EEPROM.put(address, irData);
  address += sizeof(irData);

  //アラーム無効にして保存
  AlarmSetting tempAlarm[dataMaxNum];
  for (size_t i = 0; i < dataMaxNum; i++) {
    tempAlarm[i] = alarmSetting[i];
    tempAlarm[i].isEnable = false;
  }
  EEPROM.put(address, tempAlarm);
}

// EEPROMから読み込みます
void LoadFromEEPROM() {
  int address = 0;
  EEPROM.get(address, irDataAvailables);
  address += sizeof(irDataAvailables);

  EEPROM.get(address, irData);
  address += sizeof(irData);

  EEPROM.get(address, alarmSetting);
}

//アラームモード本体
void AlarmMode() {
  lcd.clear();
  lcd.print(F("ｱﾗｰﾑ:0          "));
  lcd.cursor();
  lcd.blink();
  uint8_t alarmDataIndex = 0;
  AlarmSetting newAlarmSetting = alarmSetting[alarmDataIndex];
  lcd.setCursor(7, 0);
  if (newAlarmSetting.isEnable) {
    lcd.print(F("1:ﾕｳｺｳ"));
  } else {
    lcd.print(F("0:ﾑｺｳ "));
  }
  PrintAlarmSetting(newAlarmSetting, 0, 1);
  buttonStatus button;
  bool isCustom = false;

alarmNumInput:
  while (true) {
    lcd.setCursor(5, 0);
    button = WaitForButton();
    if (IsNumber(button)) {
      lcd.print(GetCharFromButton(button));
      alarmDataIndex = button;
      newAlarmSetting = alarmSetting[alarmDataIndex];
      lcd.setCursor(7, 0);
      if (newAlarmSetting.isEnable) {
        lcd.print(F("1:ﾕｳｺｳ"));
      } else {
        lcd.print(F("0:ﾑｺｳ "));
      }
      PrintAlarmSetting(newAlarmSetting, 0, 1);
      continue;
    } else if (button == buttonStatus::ENTER) {
      //次へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      //キャンセル・終了。
      goto finally;
    } else {
      //数字とEnterとBC以外は無視して、入力へ戻る。
      continue;
    }
  }

enableInput:
  while (true) {
    lcd.setCursor(7, 0);
    button = WaitForButton();
    if (IsNumber(button)) {
      if (button == buttonStatus::NUM1) {
        lcd.print(F("1:ﾕｳｺｳ"));
        newAlarmSetting.isEnable = true;
      } else if (button == buttonStatus::NUM0) {
        lcd.print(F("0:ﾑｺｳ "));
        newAlarmSetting.isEnable = false;
      } else {
        //なにもしない
      }
      continue;
    } else if (button == buttonStatus::ENTER) {
      //次へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      //キャンセル・終了。
      goto alarmNumInput;
    } else {
      //数字とEnterとBC以外は無視して、入力へ戻る。
      continue;
    }
  }

weekNumInput:
  PrintAlarmSetting(newAlarmSetting, 0, 1, true);
  while (true) {
    lcd.setCursor(0, 1);
    button = WaitForButton();
    if (IsNumber(button)) {
      switch (button) {
        case buttonStatus::NUM0:
          //毎日
          lcd.print(F("0ﾏｲﾆﾁ   "));
          newAlarmSetting.week = EVERY_DAY;
          break;
        case buttonStatus::NUM1:
          //平日
          lcd.print(F("1ﾍｲｼﾞﾂ  "));
          newAlarmSetting.week = WEEK_DAY;
          break;
        case buttonStatus::NUM2:
          //休日
          lcd.print(F("2ｷｭｳｼﾞﾂ "));
          newAlarmSetting.week = WEEK_END;
          break;
        case buttonStatus::NUM3:
          //一回
          newAlarmSetting.week = ONCE;
          lcd.print(F("3ｲｯｶｲ   "));
          break;
        case buttonStatus::NUM4:
          //カスタム
          newAlarmSetting.week = 0b01000000;  //適当な値
          lcd.print(F("4ｶｽﾀﾑ   "));
          break;
        default:
          //何もしない
          break;
      }
    } else if (button == buttonStatus::ENTER) {
      switch (newAlarmSetting.week) {
        case EVERY_DAY:
        case WEEK_DAY:
        case WEEK_END:
        case ONCE:
          isCustom = false;
          break;
        default:
          isCustom = true;
          break;
      }
      //次へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      //アラーム番号入力へ戻る。
      goto alarmNumInput;
    } else {
      //数字とEnterとBC以外は無視して、入力へ戻る。
      continue;
    }
  }

  //カスタムの時の曜日設定
  if (isCustom) {
    newAlarmSetting.week = SetCustomWeekSetting();
    lcd.setCursor(0, 0);
    lcd.print(F("ｱﾗｰﾑ:"));
    lcd.print(alarmDataIndex);
    lcd.print(F("          "));
  }

  PrintAlarmSetting(newAlarmSetting, 0, 1, true);

hour1Input:
  while (true) {
    lcd.setCursor(8, 1);
    button = WaitForButton();
    if (IsNumber(button)) {
      lcd.print(GetCharFromButton(button));
      newAlarmSetting.hour = (int)button * 10 + newAlarmSetting.hour % 10;
      break;
    } else if (button == buttonStatus::ENTER) {
      //次の桁へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      goto weekNumInput;
    } else {
      continue;
    }
  }

hour2Input:
  while (true) {
    lcd.setCursor(9, 1);
    button = WaitForButton();
    if (IsNumber(button)) {
      lcd.print(GetCharFromButton(button));
      newAlarmSetting.hour = newAlarmSetting.hour / 10 * 10 + (int)button;
      break;
    } else if (button == buttonStatus::ENTER) {
      //次の桁へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      goto hour1Input;
    } else {
      continue;
    }
  }

min1Input:
  while (true) {
    lcd.setCursor(11, 1);
    button = WaitForButton();
    if (IsNumber(button)) {
      lcd.print(GetCharFromButton(button));
      newAlarmSetting.minute = (int)button * 10 + newAlarmSetting.minute % 10;
      break;
    } else if (button == buttonStatus::ENTER) {
      //次の桁へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      goto hour2Input;
    } else {
      continue;
    }
  }

min2Input:
  while (true) {
    lcd.setCursor(12, 1);
    button = WaitForButton();
    if (IsNumber(button)) {
      lcd.print(GetCharFromButton(button));
      newAlarmSetting.minute = newAlarmSetting.minute / 10 * 10 + (int)button;
      break;
    } else if (button == buttonStatus::ENTER) {
      //次の桁へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      goto min1Input;
    } else {
      continue;
    }
  }

  //リモコン番号指定
irIndexInput:
  while (true) {
    lcd.setCursor(14, 1);
    button = WaitForButton();
    if (IsNumber(button)) {
      lcd.print(GetCharFromButton(button));
      newAlarmSetting.irIndex = (uint8_t)button;
      continue;
    } else if (button == buttonStatus::ENTER) {
      //次の桁へ進む。
      break;
    } else if (button == buttonStatus::BC) {
      goto min2Input;
    } else {
      continue;
    }
  }

  newAlarmSetting.isSent = false;
  alarmSetting[alarmDataIndex] = newAlarmSetting;
  SaveToEEPROM();

finally:
  lcd.noCursor();
  lcd.noBlink();
}

//指定のインデックスのアラーム設定情報をLCDに表示します。
void PrintAlarmSetting(AlarmSetting& alarm, uint8_t col, uint8_t row,
                       bool printWeekNum) {
  lcd.setCursor(col, row);
  lcd.print("                ");
  lcd.setCursor(col, row);
  PrintWeekSetName(alarm.week, printWeekNum);
  //時間
  if (alarm.hour < 10) {
    lcd.print('0');
  }
  lcd.print(alarm.hour);
  lcd.print(":");

  if (alarm.minute < 10) {
    lcd.print('0');
  }
  lcd.print(alarm.minute);

  lcd.print(F(" "));
  lcd.print(alarm.irIndex);
}

//週設定名をLCDに表示します。
void PrintWeekSetName(uint8_t week, bool printWeekNum) {
  switch (week) {
    case EVERY_DAY:
      if (printWeekNum) {
        lcd.print(F("0"));
      }
      //毎日
      lcd.print(F("ﾏｲﾆﾁ   "));
      break;
    case WEEK_DAY:
      if (printWeekNum) {
        lcd.print(F("1"));
      }
      //平日
      lcd.print(F("ﾍｲｼﾞﾂ  "));
      break;
    case WEEK_END:
      if (printWeekNum) {
        lcd.print(F("2"));
      }
      //休日
      lcd.print(F("ｷｭｳｼﾞﾂ "));
      break;
    case ONCE:
      if (printWeekNum) {
        lcd.print(F("3"));
      }
      //一回
      lcd.print(F("ｲｯｶｲ   "));
      break;
    default:
      if (printWeekNum) {
        lcd.print(F("4"));
      }
      //カスタム
      lcd.print(F("ｶｽﾀﾑ   "));
      break;
  }
}

//カスタム週設定をユーザーに促します。
uint8_t SetCustomWeekSetting() { return 0; }

//時間設定本体
void TimeSettingMode() {
  DispDateTime();
  lcd.cursor();
  lcd.blink();

  buttonStatus button;

  // RX8900は、2099年まで対応なので、下二桁の設定を行う。
year1Input:
  lcd.setCursor(2, 0);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.year = (int)button * 10 + tim.year % 10;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    //キャンセル・終了。
    lcd.noCursor();
    lcd.noBlink();
    return;
  } else {
    goto year1Input;
  }

year2Input:
  lcd.setCursor(3, 0);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.year = tim.year / 10 * 10 + (int)button;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto year1Input;
  } else {
    goto year2Input;
  }

month1Input:
  lcd.setCursor(5, 0);
  button = WaitForButton();
  if (button >= buttonStatus::NUM0 && button <= buttonStatus::NUM1) {
    lcd.print(GetCharFromButton(button));
    tim.month = (int)button * 10 + tim.month % 10;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto year2Input;
  } else {
    goto month1Input;
  }

month2Input:
  lcd.setCursor(6, 0);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.month = tim.month / 10 * 10 + (int)button;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto month1Input;
  } else {
    goto month2Input;
  }

day1Input:
  lcd.setCursor(8, 0);
  button = WaitForButton();
  if (button >= buttonStatus::NUM0 && button <= buttonStatus::NUM3) {
    lcd.print(GetCharFromButton(button));
    tim.day = (int)button * 10 + tim.day % 10;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto month2Input;
  } else {
    goto day1Input;
  }

day2Input:
  lcd.setCursor(9, 0);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.day = tim.day / 10 * 10 + (int)button;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto day1Input;
  } else {
    goto day2Input;
  }

  PrintWeekDayToLcd();

hour1Input:
  lcd.setCursor(0, 1);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.hour = (int)button * 10 + tim.hour % 10;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto day2Input;
  } else {
    goto hour1Input;
  }

hour2Input:
  lcd.setCursor(1, 1);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.hour = tim.hour / 10 * 10 + (int)button;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto hour1Input;
  } else {
    goto hour2Input;
  }

min1Input:
  lcd.setCursor(3, 1);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.minute = (int)button * 10 + tim.minute % 10;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto hour2Input;
  } else {
    goto min1Input;
  }

min2Input:
  lcd.setCursor(4, 1);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.minute = tim.minute / 10 * 10 + (int)button;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto min1Input;
  } else {
    goto min2Input;
  }

sec1Input:
  lcd.setCursor(6, 1);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.second = (int)button * 10 + tim.second % 10;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto min2Input;
  } else {
    goto sec1Input;
  }

sec2Input:
  lcd.setCursor(7, 1);
  button = WaitForButton();
  if (IsNumber(button)) {
    lcd.print(GetCharFromButton(button));
    tim.second = tim.second / 10 * 10 + (int)button;
  } else if (button == buttonStatus::ENTER) {
    //次の桁へ進む。何もしない。
  } else if (button == buttonStatus::BC) {
    goto sec1Input;
  } else {
    goto sec2Input;
  }

  RX8900.setDateTime(&tim);

  lcd.noCursor();
  lcd.noBlink();
}

// LCDに曜日を表示します。
void PrintWeekDayToLcd() {
  lcd.setCursor(10, 0);
  // time.hの機能を使用し、曜日の算出
  timeHstruct.tm_sec = 0;
  timeHstruct.tm_min = 0;
  timeHstruct.tm_hour = 0;
  timeHstruct.tm_mday = tim.day;
  timeHstruct.tm_mon = tim.month - 1;
  timeHstruct.tm_year = 2000 + tim.year - 1900;

  // mktimeを呼ぶことで、曜日が計算され、引数で渡した構造体が変更される
  mktime(&timeHstruct);

  tim.week = GetRX8900WeekDayFromTimeHData(timeHstruct.tm_wday);
  lcdNoKana.print("(");
  lcdNoKana.write(timeHstruct.tm_wday);
  lcdNoKana.print(")");
  lcdNoKana.print("   ");
}

//データ削除本体
void DeleteDataMode() {
  lcd.setCursor(0, 0);
  lcd.print(F("ﾘﾓｺﾝ･ｱﾗｰﾑﾃﾞｰﾀ ｦ "));
  lcd.setCursor(0, 1);
  lcd.print(F("ｻｸｼﾞｮ ｼﾏｽ Ok?   "));

  buttonStatus button = WaitForButton();
  if (button == buttonStatus::BC) {
    return;
  }

  // EEPROMの初期化
  lcd.setCursor(0, 0);
  lcd.print(F("ｻｸｼﾞｮﾁｭｳ        "));
  lcd.setCursor(0, 1);
  lcd.print(F("               "));

  lcd.setCursor(0, 1);
  lcd.cursor();
  lcd.blink();
  long prevProgress = 0;
  for (size_t i = 0; i < EEPROM.length(); i++) {
    EEPROM.update(i, 0);
    long progress = map(i, 0, EEPROM.length(), 0, 16);
    if ((progress - prevProgress) > 0) {
      lcd.write(0xff);
      prevProgress = progress;
    }
  }
  lcd.write(0xff);

  //グローバル変数の初期化
  irDataAvailables = 0;
  IRData ir = IRData{};
  AlarmSetting alarm = AlarmSetting{};
  for (size_t i = 0; i < dataMaxNum; i++) {
    irData[i] = ir;
    alarmSetting[i] = alarm;
  }

  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print(F("ｻｸｼﾞｮ ｶﾝﾘｮｳ     "));
  button = WaitForButton();
}

void IrSendTest() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("ｾｷｶﾞｲｾﾝﾃｽﾄ"));
  lcd.cursor();
  lcd.blink();

  while (true) {
    buttonStatus button = WaitForButton();
    if (IsNumber(button)) {
      if (IsIrDataAvailable(button)) {
        IrSender.write(&irData[button]);
      }
    } else if (button == buttonStatus::BC) {
      break;
    } else {
      //なにもしない
    }
  }

  lcd.noCursor();
  lcd.noBlink();
}

//ボタン入力を文字データに変換します。
char GetCharFromButton(buttonStatus button) {
  switch (button) {
    case buttonStatus::NUM0:
      return '0';
    case buttonStatus::NUM1:
      return '1';
    case buttonStatus::NUM2:
      return '2';
    case buttonStatus::NUM3:
      return '3';
    case buttonStatus::NUM4:
      return '4';
    case buttonStatus::NUM5:
      return '5';
    case buttonStatus::NUM6:
      return '6';
    case buttonStatus::NUM7:
      return '7';
    case buttonStatus::NUM8:
      return '8';
    case buttonStatus::NUM9:
      return '9';
    default:
      return '0';
  }
}

void SetLCDbacklight(bool isOn) { digitalWrite(LCDBacklightPin, isOn); }

// RX8900の週情報からtime.hの週情報へ変換して返します。
int8_t GetTimeHweekDayFromRX8900Data(uint8_t weekBitData) {
  switch (weekBitData) {
    case SUN:
      return _WEEK_DAYS_::SUNDAY;
    case MON:
      return _WEEK_DAYS_::MONDAY;
    case TUE:
      return _WEEK_DAYS_::TUESDAY;
    case WED:
      return _WEEK_DAYS_::WEDNESDAY;
    case THU:
      return _WEEK_DAYS_::THURSDAY;
    case FRI:
      return _WEEK_DAYS_::FRIDAY;
    case SAT:
      return _WEEK_DAYS_::SATURDAY;
    default:
      return _WEEK_DAYS_::SUNDAY;
  }
}

// time.hの週情報からRX8900の週情報へ変換します。
uint8_t GetRX8900WeekDayFromTimeHData(int8_t week) {
  switch (week) {
    case _WEEK_DAYS_::SUNDAY:
      return SUN;
    case _WEEK_DAYS_::MONDAY:
      return MON;
    case _WEEK_DAYS_::TUESDAY:
      return TUE;
    case _WEEK_DAYS_::WEDNESDAY:
      return WED;
    case _WEEK_DAYS_::THURSDAY:
      return THU;
    case _WEEK_DAYS_::FRIDAY:
      return FRI;
    case _WEEK_DAYS_::SATURDAY:
      return SAT;
    default:
      return SUN;
  }
}

//ボタンが数字かどうかを返します。
bool IsNumber(buttonStatus button) {
  return button >= buttonStatus::NUM0 && button <= buttonStatus::NUM9;
}

//指定の番号のIRDataに有効なデータが入っているどうかをかえします。
bool IsIrDataAvailable(int8_t index) {
  return (irDataAvailables & 1 << index) > 0;
}
