#ifndef RX8900_H
#define RX8900_H

#define RX8900A_ADRS           0x32

#define SEC_REG                0x00
#define MIN_REG                0x01
#define HOUR_REG               0x02
#define WEEK_REG               0x03
#define DAY_REG                0x04
#define MONTH_REG              0x05
#define YEAR_REG               0x06
#define RAM_REG                0x07
#define MIN_ALARM_REG          0x08
#define HOUR_ALARM_REG         0x09
#define WEEK_DAY_ALARM_REG     0x0A
#define Timer_CNT0_REG         0x0B
#define Timer_CNT1_REG         0x0C
#define EXTENSION_REG          0x0D
#define FLAG_REG               0x0E
#define CONTROL_REG            0x0F
#define TEMP_REG               0x17
#define BACKUP_FUNC_REG        0x18
#define NO_WEEK 0x00
#define SUN 0x01
#define MON 0x02
#define TUE 0x04
#define WED 0x08
#define THU 0x10
#define FRI 0x20
#define SAT 0x40

/*
  https://smtengkapi.com/engineer-arduino-rtc
  から引用
*/

struct dateTime {
  uint8_t second;             // 0-59
  uint8_t minute;             // 0-59
  uint8_t hour;               // 0-23
  uint8_t week;               // 0x01(Sun)-0x40(Sat)各ビットをセットする
  uint8_t day;                // 1-31
  uint8_t month;              // 1-12
  uint8_t year;               // 00-99
};

enum RTC_MODE_TYP {
  RTC_IDLE = 0,
  RTC_RX,
  RTC_RX_END,
  RTC_WRITE,
  RTC_MODE_MAX
};

class RX8900Class {
  public:
    void begin();
    void begin(struct dateTime *dt);
    void setDateTime(struct dateTime *dt);
    void getDateTime(struct dateTime *dt);
    void getTemp(uint8_t *temp);
    void setRegisters(uint8_t address, int numData, uint8_t *data);
    void getRegisters(uint8_t address, int numData, uint8_t *data);
  private:
    int decimalToBCD(int decimal);
    int BCDToDecimal(int bcd);
};
#endif

#ifdef RTC_C
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL RX8900Class RX8900;
GLOBAL RTC_MODE_TYP rtcmode;

#undef GLOBAL
