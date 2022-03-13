#pragma once

#include <RTClib.h>
#include <sqlite3.h>

void initiateIrrigation(int seconds);

extern RTC_PCF8563 rtc;
extern sqlite3 *db;
extern int g_epochNow;