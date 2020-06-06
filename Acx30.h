#pragma once

#include <stdint.h>

#define LED_1			1
#define LED_PTYPE		3
#define LED_VER_LOW		4
#define LED_VER_HIGH	5
#define LED_RED			9
#define LED_GREEN		10
#define LED_BLUE		11
#define LED_MODE		12
#define LED_CONTROL		14
#define LED_25			25

#define LED_MODE_OFF		0
#define LED_MODE_STATIC		1
#define LED_MODE_RAINBOW	2
#define LED_MODE_BREATHING	5

#define LED_UNK		0x50

bool InitSMBus();
bool ledInit();

bool SetLedMode(uint8_t mode);
bool SetLedColor(uint8_t red, uint8_t green, uint8_t blue);
