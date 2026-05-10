#include <Arduino.h>
#include "i2c.h"
#include "SSD1306_OLED.h"
#include "timekeeping.h"
#include "DS3231.h"

static SSD1306_OLED screen(0x3C);
static DS3231 rtc = DS3231();

void setup() {
    delay(100);
    I2C::begin();
    screen.init();
    rtc.init();
    rtc.setTime(5, 37, 0);
}

void loop() {
    uint8_t hours;
    uint8_t minutes;
    rtc.getTime(hours, minutes);
    Timekeeping::Time t = {hours, minutes};
    screen.setTime(t);
    screen.update();
    delay(30000);
}


#pragma region TESTS
// Scan all 7-bit I2C addresses. For each device that ACKs, display its address
// on the OLED as hex nibbles: "06:08" = 0x68, "05:07" = 0x57, "03:12" = 0x3C, etc.
// Holds each found address for 3 seconds, then moves on.
// Loops forever — replace setup() body with the normal version once address is known.
static void i2c_scan() {
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 128; addr++) {
        uint8_t dummy = 0x00;
        if (I2C::write(addr, &dummy, 1)) {
            Timekeeping::Time t = {
                (uint8_t)(addr >> 4),   // high nibble → "hours" digit
                (uint8_t)(addr & 0x0F)  // low nibble  → "minutes" digit
            };
            screen.setTime(t);
            screen.update();
            delay(3000);
            found++;
        }
    }
    if (found == 0) {
        // Nothing responded — show 00:00 as a "no devices" indicator
        Timekeeping::Time t = {0, 0};
        screen.setTime(t);
        screen.update();
    }
}

void i2c_test() {
  // test one value to see if the NACK can be detected
  Timekeeping::Time t = {0, 0};
  uint8_t dummy = 0x00;
  if (I2C::write(0x68, &dummy, 1)){
    t.hours = 6;
    t.minutes = 7;
  }
  else { 
    t.hours = 0;
    t.minutes = 0;
  }
  screen.setTime(t);
  screen.update();
}

void sda_test() {
    Timekeeping::Time t = {0, 0};
    if (I2C::sda_read() == 0) {
        t.hours = 6;
        t.minutes = 7;
    }
    else {
        t.hours = 0;
        t.minutes = 0;
    }
    screen.setTime(t);
    screen.update();
}
#pragma endregion