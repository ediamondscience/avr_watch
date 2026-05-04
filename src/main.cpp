#include <Arduino.h>
#include "i2c.h"
#include "SSD1306_OLED.h"
#include "timekeeping.h"
#include "DS3231.h"

static SSD1306_OLED screen(0x3C);

// Scan all 7-bit I2C addresses. For each device that ACKs, display its address
// on the OLED as hex nibbles: "06:08" = 0x68, "05:07" = 0x57, "03:12" = 0x3C, etc.
// Holds each found address for 3 seconds, then moves on.
// Loops forever — replace setup() body with the normal version once address is known.
static void i2c_scan() {
    screen.init();
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

void setup() {
    delay(100);
    I2C::begin();
    i2c_scan();  // <-- swap back to DS3231::init() + screen.init() once address confirmed
}

void loop() {}
