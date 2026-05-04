#ifndef TIMEKEEPING_H
#define TIMEKEEPING_H

#include <stdint.h>

class Timekeeping {
public:
    struct Time {
        uint8_t hours;   // 0-11 for 12-hour system, or 0-23 for 24-hour system
        uint8_t minutes; // 0-59
    };

    static void init(uint8_t initialHour, uint8_t initialMinute);
    static Time getCurrentTime();
    static void incrementMinute();

private:
    static Time currentTime_;
};

#endif // TIMEKEEPING_H
