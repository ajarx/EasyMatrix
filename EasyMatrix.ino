#include "common.h"
#include "preferencesUtil.h"
#include "light.h"
#include "buzzer.h"
#include "net.h"
#include "task.h"
#include "ds3231.h"

/**
EasyMatrix像素时钟  版本1.4.3

本次更新内容：

1.修复日期显示月份多一个月的bug

2024-08-28 V1.4.2
1.基于1.4.0版本适配重新设计的主控板PCB，修改按键映射关系
2.增加外部RTC时钟，主要逻辑如下：
  开机，如果有配网信息，会尝试连接网络并对时，如果联网或对时失败，则使用外部RTC时钟；
  开机，如果有配网信息，会尝试连接网络并对时，如果联网对时成功，则同步网络时间到外部RTC时间，并使用外部RTC时钟；
  开机，如果没有配网信息，则会进入配网页面；
3.修改对时页面中文图片显示为英文“GET TIME”
4.修改对时周期由5H加长为24H
5.增加开机时固件版本号串口打印
6.注意不带外部RTC芯片的无法使用此固件

注意：烧录前请将 Erase All Flash 选项选为 Enable，烧录完再重新配置网络，否则程序可能会有错误。
*/

unsigned long prevDisplay = 0;
unsigned long prevSampling = 0;
uint cnt_compare = 0; //对比时间计数

void setup() {
  Serial.begin(115200);
  // 从NVS中获取信息
  getInfos();
  // 初始化点阵屏(包含拾音器)
  initMatrix();
  //初始化IIC
  rtc_init(SDA, SCL);
  // 初始化按键
  btnInit();
  Serial.println("各外设初始化成功");
  Serial.printf("固件版本：%d.%d.%d\r\n",VERSION_1,VERSION_2,VERSION_3);
  // 创建加载动画任务
  createShowTextTask("START");
  // nvs中没有WiFi信息，进入配置页面
  if(apConfig){
    currentPage = SETTING; // 将页面置为配网页面
    wifiConfigBySoftAP(); // 开启SoftAP配置WiFi
  }else{
    // 连接WiFi,30秒超时后显示wifi连接失败的图案
    connectWiFi(30); 
    // 如果连接上了wifi,就进行NTP对时,超过30秒对时失败，改为使用外部RTC时间
    if(wifiConnected){
      checkTime(30);
    }
    if(!RTCSuccess){       
      Serial.println("时钟WIFI对时超时...使用外部RTC时间");
      sync_RTC_sysTime();
      // 屏幕显示使用RTC时间
      drawPass(3, 22, "RTC");
      delay(2000);
      // 清空屏幕
      clearMatrix();
      RTCSuccess = true;  
    }
    //显示RTC时钟和系统时钟
    rtc2mez();
    Serial.printf("RTC Time: %d-%d-%d %d:%d:%d\r\n",MEZ.jahr12,MEZ.mon12,MEZ.tag12,MEZ.std12,MEZ.min12,MEZ.sek12);
    //系统时间
    time_t now;
    time(&now);
    tm* tt;
    tt = localtime(&now);
    Serial.printf("系统时间：%s\r\n", asctime(tt)); 

    // 开启对时任务
    startTickerCheckTime();
    // RTC对时成功，并且闹钟开启后，设置闹钟倒计时
    if(clockOpen){
      startTickerClock(getClockRemainSeconds());
    }
    // 将页面置为时间页面
    currentPage = TIME; 

    if(wifiConnected){
      // 停止启动加载文字的动画
      vTaskDelete(showTextTask);
      delay(300);
      // 关闭wifi
      disConnectWifi();
    }    
    // 清屏
    clearMatrix();
    
  }
}

void loop() { 
  watchBtn();
  if(brightModel == BRIGHT_MODEL_AUTO && ((millis() - prevSampling) >= 1000 || prevSampling > millis())){
    brightSamplingValue+=analogRead(LIGHT_ADC);
    brightSamplingTime++;
    prevSampling = millis();
    if(brightSamplingTime >= BRIGHT_SAMPLING_TIMES){ // 每轮采样N次重新计算一次亮度值
      calculateBrightnessValue();
      clearBrightSampling();
    }
  }
  if(isCheckingTime){ // 对时中
    Serial.println("开始对时");
    long start = millis(); // 记录开始对时的时间
    // 绘制对时提示文字
    drawCheckTimeText();
    // 执行对时逻辑
    checkTimeTicker();
    // 将对时标志置为false
    isCheckingTime = false;
    // 让整个对时过程持续超过4秒，不然时间太短，提示文字一闪而过，让人感觉鬼畜了
    while((millis() - start) < 4000){
      delay(200);
    }
    // 清屏
    clearMatrix();
    Serial.println("结束对时");
    // 结束对时后，重新绘制之前的页面
    if(currentPage == CLOCK){
      drawClock();
    }else if(currentPage == BRIGHT){
      drawBright();
    }else if(currentPage == ANIM){
      lightedCount = 0;
      memset(matrixArray,0,sizeof(matrixArray));
    }
  }else{
    switch(currentPage){
      case SETTING:  // 配置页面
        doClient(); // 监听客户端配网请求
        break;
      case TIME: // 时钟页面       
        if((millis() - prevDisplay) >= 50 || prevDisplay > millis()){
          // 绘制时间
          drawTime(RTC_MODE);
          prevDisplay = millis();  
        }
        break;
      case RHYTHM: // 节奏灯页面
        drawRHYTHM();
        break;
      case ANIM: // 动画页面
        drawAnim();
        break;
      case CLOCK: // 闹钟设置页面
        drawClock();
        break;
      case BRIGHT: // 亮度调节页面
        drawBright();
        break;  
      default:
        break;
    }
  } 
}


