#pragma once

#include <RTClib.h>
#include "config.h"

void updateMainConfig (Config* new_config);
void initiateIrrigation(int seconds);

extern RTC_PCF8563 rtc;
extern int g_epochNow;