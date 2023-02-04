/*
 * @Description:
 * @Version: 2.0
 * @Autor: ChenYuxiang
 * @Date: 2023-01-09 20:40:41
 * @LastEditors: ChenYuxiang
 * @LastEditTime: 2023-01-10 22:59:59
 */

#include <arduino.h>
#include "MyUtils.h"
#include "Wire.h"
#include "TFT_eSPI.h"

#define PI_BUF_SIZE 128
void showImage(TFT_eSPI &tft, int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data)
{
    int32_t dx = 0;
    int32_t dy = 0;
    int32_t dw = w;
    int32_t dh = h * 2;

    if (x < 0)
    {
        dw += x;
        dx = -x;
        x = 0;
    }
    if (y < 0)
    {
        dh += y;
        dy = -y;
        y = 0;
    }

    if (dw < 1 || dh < 1)
        return;

    CS_L;

    data += dx + dy * w;

    uint16_t buffer[PI_BUF_SIZE];
    uint16_t *pix_buffer = buffer;
    uint16_t high, low;

    tft.setWindow(x, y, x + dw - 1, y + dh - 1);

    uint16_t nb = (dw * dh) / (2 * PI_BUF_SIZE);

    for (int32_t i = 0; i < nb; i++)
    {
        for (int32_t j = 0; j < PI_BUF_SIZE; j++)
        {
            high = pgm_read_word(&data[(i * 2 * PI_BUF_SIZE) + 2 * j + 1]);
            low = pgm_read_word(&data[(i * 2 * PI_BUF_SIZE) + 2 * j]);
            pix_buffer[j] = (high << 8) + low;
        }
        tft.pushPixels(pix_buffer, PI_BUF_SIZE);
    }

    uint16_t np = (dw * dh) % (2 * PI_BUF_SIZE);

    if (np)
    {
        for (int32_t i = 0; i < np; i++)
        {
            high = pgm_read_word(&data[(nb * 2 * PI_BUF_SIZE) + 2 * i + 1]);
            low = pgm_read_word(&data[(nb * 2 * PI_BUF_SIZE) + 2 * i]);
            pix_buffer[i] = (high << 8) + low;
        }
        tft.pushPixels(pix_buffer, np);
    }

    CS_H;
}

void IICScan()
{
    byte error, address;
    int nDevices;

    Serial.println("============================================================");
    Serial.println("Scanning...");
    nDevices = 0;
    for (address = 1; address < 255; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found");
    else
        Serial.println("done");
    Serial.println("============================================================");
}