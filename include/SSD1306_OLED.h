#ifndef SSD1306_OLED_H
#define SSD1306_OLED_H

#include <stdint.h>
#include "timekeeping.h"
#include "screen_images.h"

class SSD1306_OLED {
public:
    static constexpr uint8_t WIDTH  = 128;
    static constexpr uint8_t HEIGHT = 64;
    static constexpr uint8_t COMMAND_BYTE = 0x00;

    SSD1306_OLED(uint8_t address = 0x3C);

    bool init();
    bool clear();
    void setTime(Timekeeping::Time time);
    bool update();
    bool setContrast(uint8_t contrast);
    bool power(bool on);

private:
    uint8_t address_;
    Timekeeping::Time time_;
    Timekeeping::Time last_displayed_time_;
    bool needs_update_ = true;

    const oled_canvas* get_canvas_for_digit(uint8_t digit);
    bool sendCommand(uint8_t cmd);
    bool sendCommand2(uint8_t cmd, uint8_t arg);
};

#endif // SSD1306_OLED_H
