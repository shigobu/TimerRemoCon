#include <ResKeypad.h>
#include <KanaLiquidCrystal.h> // この#includeで、KanaLiquidCrystalライブラリを呼び出します。
#include <LiquidCrystal.h> // LiquidCrystalライブラリも間接的に使うので、この#includeも必要です
#include <avr/pgmspace.h>
#include <Wire.h>
#include  "RX8900.h"
#include "TimerRemoCon.h"

const int LCDBacklightPin = 4;

const int AIN2 = A0;
const int AIN1 = A1;
const int KeyNum = 7;
PROGMEM const int threshold[KeyNum] = {
  // 次の数列は、しなぷすのハード製作記の回路設計サービスで計算して得られたもの
  42,  165,  347,  511,  641,  807,  965
};
ResKeypad keypad2(AIN2, KeyNum, threshold);
ResKeypad keypad1(AIN1, KeyNum, threshold);

KanaLiquidCrystal lcd(8, 9, 10, 11, 12, 13);
LiquidCrystal lcdNoKana(8, 9, 10, 11, 12, 13);

dateTime tim;
ModeMessage message = ModeMessage::DateTime;

unsigned long prevButtonMillis = 0;
unsigned int LCDbacklightOnMillis = 5000;

#pragma region bigFont
//http://woodsgood.ca/projects/2015/02/17/big-font-lcd-characters/
const uint8_t custom[][8] PROGMEM = {                        // Custom character definitions
  { 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00 }, // char 1
  { 0x18, 0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }, // char 2
  { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07, 0x03 }, // char 3
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F }, // char 4
  { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C, 0x18 }, // char 5
  { 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F }, // char 6
  { 0x1F, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F }, // char 7
  { 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }  // char 8
};

// BIG FONT Character Set
// - Each character coded as 1-4 byte sets {top_row(0), top_row(1)... bot_row(0), bot_row(0)..."}
// - number of bytes terminated with 0x00; Character is made from [number_of_bytes/2] wide, 2 high
// - codes are: 0x01-0x08 => custom characters, 0x20 => space, 0xFF => black square

const uint8_t bigChars[][8] PROGMEM = {
  { 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Space
  { 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // !
  { 0x05, 0x05, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 }, // "
  { 0x04, 0xFF, 0x04, 0xFF, 0x04, 0x01, 0xFF, 0x01 }, // #
  { 0x08, 0xFF, 0x06, 0x07, 0xFF, 0x05, 0x00, 0x00 }, // $
  { 0x01, 0x20, 0x04, 0x01, 0x04, 0x01, 0x20, 0x04 }, // %
  { 0x08, 0x06, 0x02, 0x20, 0x03, 0x07, 0x02, 0x04 }, // &
  { 0x05, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '
  { 0x08, 0x01, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00 }, // (
  { 0x01, 0x02, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00 }, // )
  { 0x01, 0x04, 0x04, 0x01, 0x04, 0x01, 0x01, 0x04 }, // *
  { 0x04, 0xFF, 0x04, 0x01, 0xFF, 0x01, 0x00, 0x00 }, // +
  { 0x20, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //
  { 0x04, 0x04, 0x04, 0x20, 0x20, 0x20, 0x00, 0x00 }, // -
  { 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // .
  { 0x20, 0x20, 0x04, 0x01, 0x04, 0x01, 0x20, 0x20 }, // /
  { 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00 }, // 0
  { 0x01, 0x02, 0x20, 0x04, 0xFF, 0x04, 0x00, 0x00 }, // 1
  { 0x06, 0x06, 0x02, 0xFF, 0x07, 0x07, 0x00, 0x00 }, // 2
  { 0x01, 0x06, 0x02, 0x04, 0x07, 0x05, 0x00, 0x00 }, // 3
  { 0x03, 0x04, 0xFF, 0x20, 0x20, 0xFF, 0x00, 0x00 }, // 4
  { 0xFF, 0x06, 0x06, 0x07, 0x07, 0x05, 0x00, 0x00 }, // 5
  { 0x08, 0x06, 0x06, 0x03, 0x07, 0x05, 0x00, 0x00 }, // 6
  { 0x01, 0x01, 0x02, 0x20, 0x08, 0x20, 0x00, 0x00 }, // 7
  { 0x08, 0x06, 0x02, 0x03, 0x07, 0x05, 0x00, 0x00 }, // 8
  { 0x08, 0x06, 0x02, 0x07, 0x07, 0x05, 0x00, 0x00 }, // 9
  { 0xA5, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // :
  { 0x04, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ;
  { 0x20, 0x04, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00 }, // <
  { 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00 }, // =
  { 0x01, 0x04, 0x20, 0x04, 0x01, 0x01, 0x00, 0x00 }, // >
  { 0x01, 0x06, 0x02, 0x20, 0x07, 0x20, 0x00, 0x00 }, // ?
  { 0x08, 0x06, 0x02, 0x03, 0x04, 0x04, 0x00, 0x00 }, // @
  { 0x08, 0x06, 0x02, 0xFF, 0x20, 0xFF, 0x00, 0x00 }, // A
  { 0xFF, 0x06, 0x05, 0xFF, 0x07, 0x02, 0x00, 0x00 }, // B
  { 0x08, 0x01, 0x01, 0x03, 0x04, 0x04, 0x00, 0x00 }, // C
  { 0xFF, 0x01, 0x02, 0xFF, 0x04, 0x05, 0x00, 0x00 }, // D
  { 0xFF, 0x06, 0x06, 0xFF, 0x07, 0x07, 0x00, 0x00 }, // E
  { 0xFF, 0x06, 0x06, 0xFF, 0x20, 0x20, 0x00, 0x00 }, // F
  { 0x08, 0x01, 0x01, 0x03, 0x04, 0x02, 0x00, 0x00 }, // G
  { 0xFF, 0x04, 0xFF, 0xFF, 0x20, 0xFF, 0x00, 0x00 }, // H
  { 0x01, 0xFF, 0x01, 0x04, 0xFF, 0x04, 0x00, 0x00 }, // I
  { 0x20, 0x20, 0xFF, 0x04, 0x04, 0x05, 0x00, 0x00 }, // J
  { 0xFF, 0x04, 0x05, 0xFF, 0x20, 0x02, 0x00, 0x00 }, // K
  { 0xFF, 0x20, 0x20, 0xFF, 0x04, 0x04, 0x00, 0x00 }, // L
  { 0x08, 0x03, 0x05, 0x02, 0xFF, 0x20, 0x20, 0xFF }, // M
  { 0xFF, 0x02, 0x20, 0xFF, 0xFF, 0x20, 0x03, 0xFF }, // N
  { 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00 }, // 0
  { 0x08, 0x06, 0x02, 0xFF, 0x20, 0x20, 0x00, 0x00 }, // P
  { 0x08, 0x01, 0x02, 0x20, 0x03, 0x04, 0xFF, 0x04 }, // Q
  { 0xFF, 0x06, 0x02, 0xFF, 0x20, 0x02, 0x00, 0x00 }, // R
  { 0x08, 0x06, 0x06, 0x07, 0x07, 0x05, 0x00, 0x00 }, // S
  { 0x01, 0xFF, 0x01, 0x20, 0xFF, 0x20, 0x00, 0x00 }, // T
  { 0xFF, 0x20, 0xFF, 0x03, 0x04, 0x05, 0x00, 0x00 }, // U
  { 0x03, 0x20, 0x20, 0x05, 0x20, 0x02, 0x08, 0x20 }, // V
  { 0xFF, 0x20, 0x20, 0xFF, 0x03, 0x08, 0x02, 0x05 }, // W
  { 0x03, 0x04, 0x05, 0x08, 0x20, 0x02, 0x00, 0x00 }, // X
  { 0x03, 0x04, 0x05, 0x20, 0xFF, 0x20, 0x00, 0x00 }, // Y
  { 0x01, 0x06, 0x05, 0x08, 0x07, 0x04, 0x00, 0x00 }, // Z
  { 0xFF, 0x01, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00 }, // [
  { 0x01, 0x04, 0x20, 0x20, 0x20, 0x20, 0x01, 0x04 }, // Backslash
  { 0x01, 0xFF, 0x04, 0xFF, 0x00, 0x00, 0x00, 0x00 }, // ]
  { 0x08, 0x02, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 }, // ^
  { 0x20, 0x20, 0x20, 0x04, 0x04, 0x04, 0x00, 0x00 }  // _
};
uint8_t col, row, nb = 0, bc = 0;                            // general
uint8_t bb[8];                                               // byte buffer for reading from PROGMEM

// ********************************************************************************** //
//                                      SUBROUTINES
// ********************************************************************************** //
// writeBigChar: writes big character 'ch' to column x, row y; returns number of columns used by 'ch'
int writeBigChar(uint8_t ch, byte x, byte y) {
  lcd.kanaOff();
  if (ch < ' ' || ch > '_') return 0;               // If outside table range, do nothing
  nb = 0;                                           // character byte counter
  for (bc = 0; bc < 8; bc++) {
    bb[bc] = pgm_read_byte( &bigChars[ch - ' '][bc] ); // read 8 bytes from PROGMEM
    if (bb[bc] != 0) nb++;
  }

  bc = 0;
  for (row = y; row < y + 2; row++) {
    for (col = x; col < x + nb / 2; col++ ) {
      lcd.setCursor(col, row);                      // move to position
      lcd.write(bb[bc++]);                          // write byte and increment to next
    }
    //    lcd.setCursor(col, row);
    //    lcd.write(' ');                                 // Write ' ' between letters
  }
  lcd.kanaOn();
  return nb / 2 - 1;                                  // returns number of columns used by char
}

// writeBigString: writes out each letter of string
void writeBigString(uint8_t *str, byte x, byte y) {
  uint8_t c;
  while ((c = *str++))
    x += writeBigChar(c, x, y) + 1;
}

void initBogFont()
{
  lcdNoKana.begin(16, 2);
  for (nb = 0; nb < 8; nb++ ) {                 // create 8 custom characters
    for (bc = 0; bc < 8; bc++) bb[bc] = pgm_read_byte( &custom[nb][bc] );
    //カナ対応のライブラリを使用すると、うまくカスタム文字の登録ができないので、標準ライブラリの機能を使用。
    lcdNoKana.createChar ( nb + 1, bb );
  }
}

#pragma endregion

void setup() {
  pinMode(LCDBacklightPin, OUTPUT);
  pinMode(AIN1, INPUT);
  pinMode(AIN2, INPUT);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  initBogFont();

  SetLCDbacklight(true);
  lcd.clear();
  lcd.kanaOn();
  lcd.print("ﾀｲﾏｰﾘﾓｺﾝ V0.1");

  RX8900.begin();
}

void loop() {
  //lcdバックライト処理
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
    case ModeMessage::BigTime:
      DispBigTime();
      break;
    case ModeMessage::Learn:
      lcd.print("ｶﾞｸｼｭｳ ﾓｰﾄﾞ     ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      break;
    case ModeMessage::LearnDetail:
      LearnMode();
      message = ModeMessage::Learn;
      break;
    case ModeMessage::Alarm:
      lcd.print("ｱﾗｰﾑ ﾓｰﾄﾞ       ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      break;
    case ModeMessage::AlarmDetail:
      AlarmMode();
      message = ModeMessage::Alarm;
      break;
    case ModeMessage::TimeSetting:
      lcd.print("ｼﾞｶﾝ ｾｯﾃｲ       ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      break;
    case ModeMessage::TimeSettingDetail:
      TimeSettingMode();
      message = ModeMessage::DateTime;
      break;
    default:
      break;
  }
}

//モードボタンの押下状態を確認して、モードの切り替えを行います。
void SetModeMessage() {
  buttonStatus button = GetButton();
  if (button == buttonStatus::MODE) {
    switch (message) {
      case ModeMessage::DateTime:
        message = ModeMessage::BigTime;
        break;
      case ModeMessage::BigTime:
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

//LCDに時間を表示します。
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
  lcd.print("      ");

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
  lcd.print("        ");

}

//大きい時間表示を行います
void DispBigTime() {
  RX8900.getDateTime(&tim);
  //時間
  String str = "";
  if (tim.hour < 10) {
    str += '0';
  }
  str += String(tim.hour);
  str += ':';

  if (tim.minute < 10) {
    str += '0';
  }
  str += String(tim.minute);
  writeBigString(str.c_str(), 0, 0);

  lcd.setCursor(14, 1);
  if (tim.second < 10) {
    lcd.print('0');
  }
  lcd.print(tim.second);
}

//ボタンの押下状態を取得します。
buttonStatus GetButton() {
  buttonStatus button = buttonStatus::NOT_PRESSED;

  signed char key;
  key = keypad1.GetKey();
  switch (key)
  {
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
  switch (key)
  {
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
buttonStatus WaitForButton()
{
  buttonStatus button = GetButton();
  while (button == buttonStatus::NOT_PRESSED) {
    button = GetButton();
  }
  return button;
}

//学習モード本体
void LearnMode()
{
  lcd.setCursor(0, 1);
  lcd.print("ﾅｶﾐ            ");
  delay(1000);
}

//アラームモード本体
void AlarmMode()
{
  lcd.setCursor(0, 1);
  lcd.print("ﾅｶﾐ            ");
  delay(1000);
}

//時間設定本体
void TimeSettingMode()
{
  DispDateTime();
  lcd.cursor();
  lcd.blink();

  dateTime dt;
  buttonStatus button;

  //RX8900は、2099年まで対応なので、下二桁の設定を行う。
year1Input:
  lcd.setCursor(2, 0);
  button = WaitForButton();
  if (button >= buttonStatus::NUM0 && button <= buttonStatus::NUM9) {
    lcd.print(GetCharFromButton(button));
    dt.year = (int)button * 10;
  }
  // else if(button == buttonStatus::ENTER){
  //   lcd.setCursor(3,0);
  // }
  else if (button == buttonStatus::BC) {
    //キャンセル・終了。
    return;
  }
  else {
    goto year1Input;
  }

year2Input:
  lcd.setCursor(3, 0);
  button = WaitForButton();
  if (button >= buttonStatus::NUM0 && button <= buttonStatus::NUM9) {
    lcd.print(GetCharFromButton(button));
    dt.year += (int)button;
  }
  else if (button == buttonStatus::BC) {
    goto year1Input;
  }
  else {
    goto year2Input;
  }

month1Input:
  lcd.setCursor(5, 0);
  button = WaitForButton();
  if (button >= buttonStatus::NUM0 && button <= buttonStatus::NUM1) {
    lcd.print(GetCharFromButton(button));
    dt.month = (int)button * 10;
  }
  else if (button == buttonStatus::BC) {
    goto year2Input;
  }
  else {
    goto month1Input;
  }

  WaitForButton();
  lcd.noCursor();
  lcd.noBlink();
}

//ボタン入力を文字データに変換します。
char GetCharFromButton(buttonStatus button)
{
  switch (button)
  {
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
