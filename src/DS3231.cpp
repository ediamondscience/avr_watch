#include "DS3231.h"
#include "i2c.h"

uint8_t DS3231::encodeBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

uint8_t DS3231::decodeBcd(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

bool DS3231::init() {
    uint8_t status;
    if (!I2C::readRegister(ADDRESS, 0x0F, status)) return false;

    if (status & 0x80) {
        if (!setTime(11, 55, 0)) return false;
        if (!I2C::writeRegister(ADDRESS, 0x0F, status & ~0x80)) return false;
    }
    return true;
}

bool DS3231::setTime(uint8_t hours24, uint8_t minutes, uint8_t seconds) {
    uint8_t data[8] = {
        0x00,                  // register pointer → seconds register (0x00)
        encodeBcd(seconds),    // reg 0x00: seconds
        encodeBcd(minutes),    // reg 0x01: minutes
        encodeBcd(hours24),    // reg 0x02: hours — bit6=0 selects 24hr mode
        0x01,                  // reg 0x03: day-of-week (unused, placeholder)
        0x01,                  // reg 0x04: date
        0x01,                  // reg 0x05: month
        0x00,                  // reg 0x06: year
    };
    return I2C::write(ADDRESS, data, 8);
}

bool DS3231::getTime(uint8_t &hours24, uint8_t &minutes) {
    // Set register pointer to 0x00 (seconds)
    uint8_t reg = 0x00;
    if (!I2C::write(ADDRESS, &reg, 1)) return false;

    // Read seconds (0x00), minutes (0x01), hours (0x02)
    uint8_t buf[3];
    if (!I2C::read(ADDRESS, buf, 3)) return false;

    minutes = decodeBcd(buf[1] & 0x7F);
    hours24 = decodeBcd(buf[2] & 0x3F); // mask out 12/24 and 20hr bits
    return true;
}
