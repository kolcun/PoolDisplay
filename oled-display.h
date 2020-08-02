#ifndef OLED-DISPLAY_H
#define OLED-DISPLAY_H

//OLED size
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128 

//Pins for the OLED Display
#define SCLK_PIN D5 // yellow wire
#define MOSI_PIN D7 // blue wire
#define DC_PIN   D2 // green wire
#define CS_PIN   D8 // orange wire
#define RST_PIN  D0 // white wire

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
#define GREY            0x8410

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

#endif
