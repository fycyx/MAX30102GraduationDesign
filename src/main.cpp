/*
 * @Description:
 * @Version: 2.0
 * @Autor: ChenYuxiang
 * @Date: 2023-01-08 23:27:43
 * @LastEditors: ChenYuxiang
 * @LastEditTime: 2023-02-04 22:29:42
 */
#include <Arduino.h>
#include "TFT_eSPI.h"
#include <SPI.h>

#include "algorithm_by_RF.h"
#include "max30102.h"
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "MyUtils.h"

#include "logo2.h"
#include "logo1.h"

#define OUT_TICKS (pdMS_TO_TICKS(1000))
#define MAX30102_DELAY (pdMS_TO_TICKS(500))
#define RESET_TIMER0 (15)
#define DISPLAY_X (0)
#define DISPLAY_Y (60)
#define DISPLAY_LINE (3)
#define GPIO_INT GPIO_NUM_13

#define DEBUG Serial

/*---------------VAR DEFINE-----------------*/
MAX_Data maxData;
TaskHandle_t max30102Hand = NULL;
TaskHandle_t lcdShowHand = NULL;
TaskHandle_t serialPrintHand = NULL;
TimerHandle_t timer0Hand = NULL;

TFT_eSPI tft = TFT_eSPI();
bool isFirstMeasur = true;
bool flushLcd = false;
/*---------------VAR DEFINE END-------------*/

/*---------------FUN DEFINE-----------------*/
void takeMax30102Value(void *p);
void lcdShowData(void *p);
void serialPrint(void *p);

void showValue(int GFXStep, bool flush);
void showNoFinger(int GFXStep);
void showFinger(int GFXStep);
void showStartLogo();

void timer0FlushValue(void *);
/*---------------FUN DEFINE END--------------*/

void setup()
{

  timer0Hand = xTimerCreate("flushLcdData", pdMS_TO_TICKS(pdMS_TO_TICKS(1000) * RESET_TIMER0), pdFALSE, 0, timer0FlushValue);
  xTaskCreatePinnedToCore(lcdShowData, "lcdShowFun", 1024 * 10, NULL, 2, &lcdShowHand, 1);
  xTaskCreatePinnedToCore(takeMax30102Value, "MAX30102", 1024 * 10, NULL, 2, &max30102Hand, 1);
  xTaskCreatePinnedToCore(serialPrint, "serialPrint", 1024 * 10, NULL, 1, &serialPrintHand, 1);

  vTaskDelete(NULL);
}

void serialPrint(void *p)
{
  int SemValue = 0;
  TickType_t curTime = xTaskGetTickCount();

  DEBUG.begin(115200);
  DEBUG.print("GoTo Measuring\r\n");

  while (1)
  {
    DEBUG.println(F("SpO2\tHR\tRatio\tCorr\tisF\tdOK"));
    DEBUG.print(maxData.spo2);
    DEBUG.print("\t");
    DEBUG.print(maxData.heartrate, DEC);
    DEBUG.print("\t");
    DEBUG.print(maxData.ratio);
    DEBUG.print("\t");
    DEBUG.print(maxData.correl);
    DEBUG.print("\t");
    DEBUG.print(maxData.isFinger);
    DEBUG.print("\t");
    DEBUG.print(maxData.dataOK);
    DEBUG.println("");

    vTaskDelayUntil(&curTime, 1000);
  }
}

void lcdShowData(void *p)
{
  int GFXStep = 16;
  int SemValue = 0;

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  showStartLogo();

  showImage(tft, 0, 0, 159, 43, logo2);

  showFinger(GFXStep);

  while (1)
  {
    // vTaskDelay(100);

    if (ulTaskNotifyTake(true, OUT_TICKS))
    {
      if (maxData.isFinger == false)
      {
        showNoFinger(GFXStep);
      }
      else if (maxData.dataOK == false && maxData.isFinger == true)
      {
        showFinger(GFXStep);
      }

      if (maxData.usedFlag == false && flushLcd == false && maxData.dataOK == true)
      {
        showValue(GFXStep, false);

        if (xTimerIsTimerActive(timer0Hand) != pdFALSE)
        {
          xTimerStop(timer0Hand, OUT_TICKS);
        }
      }

      if (flushLcd)
      {
        showValue(GFXStep, flushLcd);
        flushLcd = false;
      }
    }
  }
}

void showStartLogo()
{
  showImage(tft, 0, 0, 160, 128, logo1);

  while ((ulTaskNotifyTake(true, ULONG_MAX) == 0) || !maxData.isFinger)
  {
    vTaskDelay(10);
  }
}

void timer0FlushValue(void *p)
{
  flushLcd = true;
  xTaskNotifyGive(lcdShowHand);
}

void showFinger(int GFXStep)
{
  if (isFirstMeasur)
  {
    tft.fillRect(DISPLAY_X, GFXStep * 3 - 5, 160, 100, TFT_BLUE);
    tft.setFreeFont(&FreeSansOblique9pt7b);
    tft.setTextSize(3);
    tft.setCursor(-2, 100);
    tft.setTextColor(TFT_YELLOW);
    tft.printf("Geting");
  }
  else
  {
    tft.fillRect(DISPLAY_X, 98, 160, 33, TFT_BLUE);
    tft.setCursor(0, 117);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.printf("      Measuring...");
  }
}

void showNoFinger(int GFXStep)
{
  if (isFirstMeasur)
  {
    tft.fillRect(DISPLAY_X, GFXStep * 3 - 5, 160, 100, TFT_BLUE);
    tft.setFreeFont(&FreeSerifBoldItalic9pt7b);
    tft.setTextSize(2);
    tft.setCursor(0, 68);
    tft.setTextColor(TFT_RED);
    tft.printf("     Put");
    tft.setCursor(0, 88);
    tft.setTextColor(TFT_RED);
    tft.printf("     you");
    tft.setCursor(0, 120);
    tft.setTextColor(TFT_RED);
    tft.printf("   finger");
  }
  else
  {
    tft.fillRect(DISPLAY_X, 98, 160, 33, TFT_BLUE);
    tft.setCursor(0, 117);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(TFT_RED);
    tft.printf("    Put you finger...");

    if (xTimerIsTimerActive(timer0Hand) == pdFALSE)
    {
      xTimerStart(timer0Hand, OUT_TICKS);
    }
  }
}

void showValue(int GFXStep, bool flush)
{
  if (flush)
  {
    maxData.spo2 = maxData.heartrate = maxData.correl = 0;
  }
  else
  {
    tft.fillRect(DISPLAY_X, 98, 160, 33, TFT_BLUE);
  }
  tft.fillRect(DISPLAY_X, GFXStep * 3 - 5, 160, 10 + DISPLAY_LINE * 15, TFT_RED);

  tft.setCursor(DISPLAY_X, DISPLAY_Y);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK);
  tft.print("    SPO2  : ");
  tft.setTextColor(maxData.spo2 > 95 ? TFT_GREEN : TFT_BLACK);
  tft.printf("%.2lf %%", maxData.spo2);

  tft.setCursor(DISPLAY_X, DISPLAY_Y + GFXStep * 1);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK);
  tft.print("     BMP   : ");
  tft.setTextColor(maxData.heartrate > 60 ? TFT_GREEN : TFT_BLACK);
  tft.printf("%2d", maxData.heartrate);

  tft.setCursor(DISPLAY_X, DISPLAY_Y + GFXStep * 2);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK);
  tft.print("   CORR  :");
  tft.setTextColor(maxData.correl * 100 > 90.0 ? TFT_GREEN : TFT_BLACK);
  tft.printf(" %2d %%", (int32_t)(maxData.correl * 100));

  maxData.dataOK = false;
  maxData.usedFlag = true;
  isFirstMeasur = false;
}

void takeMax30102Value(void *p)
{
  uint32_t aun_ir, aun_red;
  uint32_t aun_ir_buffer[BUFFER_SIZE];
  uint32_t aun_red_buffer[BUFFER_SIZE];
  const byte oxiInt = GPIO_INT;
  TickType_t cutTick = xTaskGetTickCount();
  int8_t spo2_ok, hr_ok;
  int threshold = 80000;
  int iter = 0;

  spo2_ok = hr_ok = maxData.dataOK = false;
  maxData.isFinger = true;

  Wire.begin();
  // IICScan();

  pinMode(oxiInt, INPUT);
  maxim_max30102_reset();
  maxim_max30102_read_reg(REG_INTR_STATUS_1, NULL);
  maxim_max30102_init();

  while (1)
  {
    for (iter = 0; iter < BUFFER_SIZE; iter++)
    {
      while (digitalRead(oxiInt) == 1)
        vTaskDelay(pdMS_TO_TICKS(1));

      maxim_max30102_read_fifo(&aun_ir, &aun_red);

      if (aun_ir < threshold)
      {
        if (maxData.isFinger != false)
        {
          maxData.isFinger = false;
          maxData.dataOK = false;
          maxData.usedFlag == false;
          xTaskNotifyGive(lcdShowHand);
        }

        maxim_max30102_reset();
        vTaskDelayUntil(&cutTick, MAX30102_DELAY);
        maxim_max30102_init();
        break;
      }
      else
      {
        cutTick = xTaskGetTickCount();

        if (maxData.isFinger != true)
        {
          maxData.isFinger = true;
          maxData.dataOK = false;
          maxData.usedFlag == false;
          xTaskNotifyGive(lcdShowHand);
        }
      }

      vTaskDelay(pdMS_TO_TICKS(1000 / FS));

      *(aun_ir_buffer + iter) = aun_ir;
      *(aun_red_buffer + iter) = aun_red;
    }

    if (iter >= BUFFER_SIZE)
    {
      rf_heart_rate_and_oxygen_saturation(
          aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &(maxData.spo2), &spo2_ok, &(maxData.heartrate), &hr_ok, &(maxData.ratio), &(maxData.correl));
    }

    if (spo2_ok && hr_ok)
    {
      maxData.usedFlag = false;
      maxData.dataOK = true;
      xTaskNotifyGive(lcdShowHand);
      spo2_ok = hr_ok = false;
    }
  }
}

void loop()
{
}
