#ifndef TIMER_REMO_CON_H
#define TIMER_REMO_CON_H

enum buttonStatus{
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

enum ModeMessage{
  DateTime,
  BigTime,
  Learn,
  Alarm,
  TimeSetting,
  LearnDetail,
  AlarmDetail,
  TimeSettingDetail,
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

#endif