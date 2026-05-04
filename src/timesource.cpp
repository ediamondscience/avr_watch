#include "timesource.h"
#include "timekeeping.h"
#include <Arduino.h>

static unsigned long lastMinuteMillis = 0;

bool minute_ticker() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastMinuteMillis >= 60000) {
        lastMinuteMillis = currentMillis;
        Timekeeping::incrementMinute();
        return true;
    }
    return false;
}
