#include <Arduino.h>
#include "common.h"
#include <time.h>
#include <Wire.h>
#include "ds3231.h"
#include <sys/time.h>

unsigned char rtc_Read(unsigned char regaddress);
void rtc_Write(unsigned char regaddress, unsigned char value);

const unsigned char DS3231_ADDRESS = 0x68;
const unsigned char secondREG = 0x00;
const unsigned char minuteREG = 0x01;
const unsigned char hourREG = 0x02;
const unsigned char WTREG = 0x03;                   //weekday
const unsigned char dateREG = 0x04;
const unsigned char monthREG = 0x05;
const unsigned char yearREG = 0x06;
const unsigned char alarm_min1secREG = 0x07;
const unsigned char alarm_min1minREG = 0x08;
const unsigned char alarm_min1hrREG = 0x09;
const unsigned char alarm_min1dateREG = 0x0A;
const unsigned char alarm_min2minREG = 0x0B;
const unsigned char alarm_min2hrREG = 0x0C;
const unsigned char alarm_min2dateREG = 0x0D;
const unsigned char controlREG = 0x0E;
const unsigned char statusREG = 0x0F;
const unsigned char ageoffsetREG = 0x10;
const unsigned char tempMSBREG = 0x11;
const unsigned char tempLSBREG = 0x12;
const unsigned char _24_hour_format = 0;
const unsigned char _12_hour_format = 1;
const unsigned char AM = 0;
const unsigned char PM = 1;

// struct DateTime {
//     unsigned short sek1, sek2, sek12, min1, min2, min12, std1, std2, std12;
//     unsigned short tag1, tag2, tag12, mon1, mon2, mon12, jahr1, jahr2, jahr12, WT;
// } MEZ;



TIME_MEZ MEZ;


//**************************************************************************************************
void rtc_init(unsigned char sda, unsigned char scl) {
    Wire.begin(sda, scl);
    rtc_Write(controlREG, 0x00);
}
//**************************************************************************************************
// BCD Code
//**************************************************************************************************
unsigned char dec2bcd(unsigned char x) { //value 0...99
    unsigned char z, e, r;
    e = x % 10;
    z = x / 10;
    z = z << 4;
    r = e | z;
    return (r);
}
unsigned char bcd2dec(unsigned char x) { //value 0...99
    int z, e;
    e = x & 0x0F;
    z = x & 0xF0;
    z = z >> 4;
    z = z * 10;
    return (z + e);
}
//**************************************************************************************************
// RTC I2C Code
//**************************************************************************************************
unsigned char rtc_Read(unsigned char regaddress) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(regaddress);
    Wire.endTransmission();
    Wire.requestFrom((unsigned char) DS3231_ADDRESS, (unsigned char) 1);
    return (Wire.read());
}
void rtc_Write(unsigned char regaddress, unsigned char value) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(regaddress);
    Wire.write(value);
    Wire.endTransmission();
}
//**************************************************************************************************
unsigned char rtc_sekunde() {
    return (bcd2dec(rtc_Read(secondREG)));
}
unsigned char rtc_minute() {
    return (bcd2dec(rtc_Read(minuteREG)));
}
unsigned char rtc_stunde() {
    return (bcd2dec(rtc_Read(hourREG)));
}
unsigned char rtc_wochentag() {
    return (bcd2dec(rtc_Read(WTREG)));
}
unsigned char rtc_tag() {
    return (bcd2dec(rtc_Read(dateREG)));
}
unsigned char rtc_monat() {
    return (bcd2dec(rtc_Read(monthREG)));
}
unsigned char rtc_jahr() {
    return (bcd2dec(rtc_Read(yearREG)));
}
void rtc_sekunde(unsigned char sek) {
    rtc_Write(secondREG, (dec2bcd(sek)));
}
void rtc_minute(unsigned char min) {
    rtc_Write(minuteREG, (dec2bcd(min)));
}
void rtc_stunde(unsigned char std) {
    rtc_Write(hourREG, (dec2bcd(std)));
}
void rtc_wochentag(unsigned char wt) {
    rtc_Write(WTREG, (dec2bcd(wt)));
}
void rtc_tag(unsigned char tag) {
    rtc_Write(dateREG, (dec2bcd(tag)));
}
void rtc_monat(unsigned char mon) {
    rtc_Write(monthREG, (dec2bcd(mon)));
}
void rtc_jahr(unsigned char jahr) {
    rtc_Write(yearREG, (dec2bcd(jahr)));
}
//**************************************************************************************************
void rtc_set(tm* tt) {
    rtc_sekunde((unsigned char) tt->tm_sec);
    rtc_minute((unsigned char) tt->tm_min);
    rtc_stunde((unsigned char) tt->tm_hour);
    rtc_tag((unsigned char) tt->tm_mday);
    rtc_monat((unsigned char) tt->tm_mon + 1);
    rtc_jahr((unsigned char) tt->tm_year - 100);
    if (tt->tm_wday == 0)
    {
      rtc_wochentag(7);
    }
    else
    rtc_wochentag((unsigned char) tt->tm_wday);
}
//**************************************************************************************************
float rtc_temp() {
    float t = 0.0;
    unsigned char lowByte = 0;
    signed char highByte = 0;
    lowByte = rtc_Read(tempLSBREG);
    highByte = rtc_Read(tempMSBREG);
    lowByte >>= 6;
    lowByte &= 0x03;
    t = ((float) lowByte);
    t *= 0.25;
    t += highByte;
    return (t); // return temp value
}
//**************************************************************************************************
void rtc2mez() {
 
    unsigned short Jahr, Tag, Monat, WoTag, Stunde, Minute, Sekunde;

    Jahr = rtc_jahr();//年
    if (Jahr > 99)
        Jahr = 0;
    Monat = rtc_monat();//月
    if (Monat > 12)
        Monat = 0;
    Tag = rtc_tag();//日
    if (Tag > 31)
        Tag = 0;
    WoTag = rtc_wochentag();//星期
    if (WoTag == 7)
        WoTag = 0;
    Stunde = rtc_stunde();//小时
    if (Stunde > 23)
        Stunde = 0;
    Minute = rtc_minute();//分钟
    if (Minute > 59)
        Minute = 0;
    Sekunde = rtc_sekunde();//秒
    if (Sekunde > 59)
        Sekunde = 0;
    
    MEZ.WT = WoTag;          //So=0, Mo=1, Di=2 ...
    MEZ.sek1 = Sekunde % 10;
    MEZ.sek2 = Sekunde / 10;
    MEZ.sek12 = Sekunde;
    MEZ.min1 = Minute % 10;
    MEZ.min2 = Minute / 10;
    MEZ.min12 = Minute;
    MEZ.std1 = Stunde % 10;
    MEZ.std2 = Stunde / 10;
    MEZ.std12 = Stunde;
    MEZ.tag12 = Tag;
    MEZ.tag1 = Tag % 10;
    MEZ.tag2 = Tag / 10;
    MEZ.mon12 = Monat;
    MEZ.mon1 = Monat % 10;
    MEZ.mon2 = Monat / 10;
    MEZ.jahr12 = Jahr;
    MEZ.jahr1 = Jahr % 10;
    MEZ.jahr2 = Jahr / 10;

    // Serial.printf("Time: %d-%d-%d %d:%d:%d\r\n",MEZ.jahr12,MEZ.mon12,MEZ.tag12,MEZ.std12,MEZ.min12,MEZ.sek12);
}


//同步外部RTC时间到系统时间
//设置系统时间
void setTime(int sc, int mn, int hr, int dy, int mt, int yr) {
    // seconds, minute, hour, day, month, year $ microseconds(optional)
    // ie setTime(20, 34, 8, 1, 4, 2021) = 8:34:20 1/4/2021
    struct tm t = {0};        // Initalize to all 0's
    t.tm_year = yr + 2000 - 1900;    // This is year-1900, so 121 = 2021
    t.tm_mon = mt - 1;
    t.tm_mday = dy;
    t.tm_hour = hr;
    t.tm_min = mn;
    t.tm_sec = sc;
    time_t timeSinceEpoch = mktime(&t);
    //   setTime(timeSinceEpoch, ms);
    // struct timeval now = { .tv_sec = timeSinceEpoch };
    struct timeval now = { .tv_sec = timeSinceEpoch };
    settimeofday(&now, NULL);
}
void sync_RTC_sysTime() {
  rtc2mez();
  setTime(MEZ.sek12, MEZ.min12, MEZ.std12, MEZ.tag12, MEZ.mon12, MEZ.jahr12);
}

