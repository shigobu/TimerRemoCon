#include <KanaLiquidCrystal.h>  // この#includeで、KanaLiquidCrystalライブラリを呼び出します。
#include <LiquidCrystal.h>  // LiquidCrystalライブラリも間接的に使うので、この#includeも必要です
#include <ResKeypad.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include <time.h>

#define NO_LED_FEEDBACK_CODE
#include <IRremote.hpp>

#include "RX8900.h"
#include "TimerRemoCon.h"

const int LCDBacklightPin = 4;

const int AIN2 = A0;
const int AIN1 = A1;
const int KeyNum = 7;
PROGMEM const int threshold[KeyNum] = {
  // 次の数列は、しなぷすのハード製作記の回路設計サービスで計算して得られたもの
  42, 165, 347, 511, 641, 807, 965
};
ResKeypad keypad2(AIN2, KeyNum, threshold);
ResKeypad keypad1(AIN1, KeyNum, threshold);

KanaLiquidCrystal lcd(8, 9, 10, 11, 12, 13);
LiquidCrystal lcdNoKana(8, 9, 10, 11, 12, 13);

dateTime tim;
ModeMessage message = ModeMessage::DateTime;

unsigned long prevButtonMillis = 0;
unsigned int LCDbacklightOnMillis = 5000;

const int IR_RECEIVE_PIN = 2;
const int IR_SEND_PIN = 3;
IRData irData[10];
// irData配列の各要素に有効な値が入っているかどうかをフラグで表している。下桁から順に格納されている。
int16_t irDataAvailables = 0;

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
  IrSender.begin(IR_SEND_PIN, false);
  lcd.setCursor(0, 0);
  lcd.print(F("ﾀｲﾏｰﾘﾓｺﾝ V0.1"));
  delay(1000);
}

void loop() {
  // lcdバックライト処理
  unsigned long span = millis() - prevButtonMillis;
  if (span > LCDbacklightOnMillis) {
    SetLCDbacklight(false);
  }

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
      message = ModeMessage::Learn;
      break;
    case ModeMessage::Alarm:
      lcd.print(F("ｱﾗｰﾑ ﾓｰﾄﾞ       "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    case ModeMessage::AlarmDetail:
      AlarmMode();
      message = ModeMessage::Alarm;
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
    default:
      break;
  }
}

void initWeekFont() {
  uint8_t bb[8] = {0};
  for (int nb = 0; nb < 7; nb++) {
    for (int bc = 0; bc < 8; bc++) bb[bc] = pgm_read_byte(&weekFont[nb][bc]);
    //カナ対応のライブラリを使用すると、うまくカスタム文字の登録ができないので、標準ライブラリの機能を使用。
    lcdNoKana.createChar(nb + 1, bb);
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
      default:
        break;
    }
  }
}

//モードメッセージを取得します。
ModeMessage GetModeMessage() {
  return message;
}

// LCDに時間を表示します。
void DispDateTime() {
  RX8900.getDateTime(&tim);
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
  lcd.print(":");

  if (tim.second < 10) {
    lcd.print('0');
  }
  lcd.print(tim.second);
  lcd.print(F("        "));
}

//ボタンの押下状態を取得します。
buttonStatus GetButton() {
  buttonStatus button = buttonStatus::NOT_PRESSED;

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
  //ボタンが押されたら、現在時間を記録
  if (button != buttonStatus::NOT_PRESSED) {
    SetLCDbacklight(true);
    prevButtonMillis = millis();
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

  int8_t irDataIndex = 0;
  if (IsIrDataAvailable(irDataIndex)) {
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
    irDataIndex = button;
    if (IsIrDataAvailable(irDataIndex)) {
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
  lcd.print(irDataIndex);
  lcd.print(F(" ﾊﾞﾝﾆﾄｳﾛｸｼﾏｽ  "));

  IrReceiver.start(80000);
  //赤外線信号受信待機
  while (!IrReceiver.decode()) {
    ;
  }

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
    irData[irDataIndex] = IrReceiver.decodedIRData;
    irDataAvailables |= 1 << irDataIndex;
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

//アラームモード本体
void AlarmMode() {
  //テストコード
  lcd.clear();
  lcd.print(F("ｿｳｼﾝ ﾃｽﾄ        "));

  int8_t irDataIndex = 0;

  buttonStatus button = WaitForButton();
  if (IsNumber(button)) {
    irDataIndex = button;
    IrSender.write(&irData[irDataIndex], 3);
  }
  delay(1000);
}

//時間設定本体
void TimeSettingMode() {
  DispDateTime();
  lcd.cursor();
  lcd.blink();

  RX8900.getDateTime(&tim);
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
  tm timeHstruct;
  timeHstruct.tm_sec = 0;
  timeHstruct.tm_min = 0;
  timeHstruct.tm_hour = 1;
  timeHstruct.tm_mday = tim.day;
  timeHstruct.tm_mon = tim.month - 1;
  timeHstruct.tm_year = 2000 + tim.year - 1900;
  // mktimeを呼ぶことで、曜日が計算され、引数で渡した構造体が変更される
  mktime(&timeHstruct);

  tim.week = GetRX8900WeekDayFromTimeHData(timeHstruct.tm_wday);
  lcd.print("(");
  lcd.write(timeHstruct.tm_wday + 1);
  lcd.print(")");
  lcd.print("   ");
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

void SetLCDbacklight(bool isOn) {
  digitalWrite(LCDBacklightPin, isOn);
}

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
