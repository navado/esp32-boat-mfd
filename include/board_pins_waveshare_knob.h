#pragma once

// clang-format off

// Waveshare ESP32-S3-Knob-Touch-LCD-1.8 (module JC3636K518)
// 1.8" 360x360 round, ST77916 QSPI panel, CST816S touch, rotary encoder.
// ESP32-S3R8 (16 MB flash + 8 MB PSRAM). Audio second-ESP32 unused.
// Pin map verified: github.com/arendst/Tasmota/discussions/23737,
// github.com/orgs/esphome/discussions/3253.

#ifndef LCD_W
#define LCD_W   360
#endif
#ifndef LCD_H
#define LCD_H   360
#endif

// ST77916 over Quad-SPI (single-data CS/CLK + D0..D3).
#define QSPI_CS    14
#define QSPI_SCK   13
#define QSPI_D0    15
#define QSPI_D1    16
#define QSPI_D2    17
#define QSPI_D3    18
#define LCD_RST    21
#define LCD_BL     47     // backlight, LEDC PWM (active high)

// CST816S capacitive touch (I2C, addr 0x15).
#define TOUCH_SDA  11
#define TOUCH_SCL  12
#ifndef TOUCH_INT
#define TOUCH_INT  9
#endif
#define TOUCH_RST  10
#ifndef TOUCH_INT_ACTIVE_LOW
#define TOUCH_INT_ACTIVE_LOW 1
#endif

// Rotary encoder (quadrature) + push switch.
#define ENC_A      8
#define ENC_B      7
#define ENC_BTN    0      // also boot-strap; input w/ pull-up after boot

// DRV2605 haptic on the touch I2C bus (addr 0x5A).
#define HAPTIC_I2C_ADDR 0x5A

// clang-format on
