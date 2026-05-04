#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>

class DS3231 {
public:
    static constexpr uint8_t ADDRESS = 0x68;

    // If OSF flag is set (clock lost power), writes 11:55:00 and clears the flag.
    // Otherwise leaves the running time alone.
    static bool init();

    static bool setTime(uint8_t hours24, uint8_t minutes, uint8_t seconds = 0);
    static bool getTime(uint8_t &hours24, uint8_t &minutes);

private:
    static uint8_t encodeBcd(uint8_t val);
    static uint8_t decodeBcd(uint8_t bcd);
};

#endif // DS3231_H
