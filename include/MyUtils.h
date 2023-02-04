/*
 * @Description:
 * @Version: 2.0
 * @Autor: ChenYuxiang
 * @Date: 2023-01-09 20:40:54
 * @LastEditors: ChenYuxiang
 * @LastEditTime: 2023-01-10 23:38:53
 */
#ifndef MYUTILS_H_
#define MYUTILS_H_

#include <arduino.h>
#include "TFT_eSPI.h"

typedef struct
{
    float spo2, ratio, correl;
    int32_t heartrate;
    bool usedFlag;
    bool isFinger;
    bool dataOK;
} MAX_Data;

void IICScan();
void showImage(TFT_eSPI &tft, int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data); 

#endif