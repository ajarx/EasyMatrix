#ifndef __DS3231_H
#define __DS3231_H


typedef struct {
  unsigned short sek1, sek2, sek12, min1, min2, min12, std1, std2, std12;
  unsigned short tag1, tag2, tag12, mon1, mon2, mon12, jahr1, jahr2, jahr12, WT;
}TIME_MEZ;

extern TIME_MEZ MEZ;

void rtc_init(unsigned char sda, unsigned char scl);
void rtc_set(tm* tt);
void rtc2mez();
void sync_RTC_sysTime();

#endif
