// SSD1306_OLED.cpp
//
// Minimal I2C-based driver class for a 128x64 GME OLED-like module.
// Assumes an I2C class with a method having the signature:
//   bool write(uint8_t address, const uint8_t *data, size_t length);
// Adapt the I2C calls to your project's I2C API if names/arguments differ.
//
// This implementation uses command/data control bytes (0x00 command, 0x40 data)
// and writes the framebuffer one 128-byte page at a time (8 pages for 64px).
// The init sequence below is basic/typical; adjust to the exact controller
// (SSD1306/SH1106/etc.) as required.
//
// This version is optimized for low RAM usage, forgoing a framebuffer in RAM.

#include <stdint.h>
#include "i2c.h"
#include "SSD1306_OLED.h"

// The PROGMEM attribute and accompanying macros are from AVR Libc
#include <avr/pgmspace.h>


SSD1306_OLED::SSD1306_OLED(uint8_t address)
    : address_(address) {
    // Initialize time to a known state
    time_.hours = 0;
    time_.minutes = 0;
    last_displayed_time_.hours = 255; // Force update on first call
    last_displayed_time_.minutes = 255;
}

// Initialize display (basic sequence; adapt for exact controller)
bool SSD1306_OLED::init() {
    if (!sendCommand(0xAE)) return false;             // Set display off
    if (!sendCommand2(0xA8, 0x3F)) return false;      // Set display mux ratio
    if (!sendCommand2(0xD3, 0x00)) return false;      // Set display offset
    if (!sendCommand(0x40)) return false;             // Set start line = 0
    if (!sendCommand(0xA1)) return false;             // Segment remap
    if (!sendCommand(0xC8)) return false;             // COM output scan direction
    if (!sendCommand2(0xDA, 0x12)) return false;      // COM pins hardware configuration
    if (!sendCommand2(0x81, 0x7F)) return false;      // Contrast
    if (!sendCommand(0xA4)) return false;             // Entire display ON resume
    if (!sendCommand(0xA6)) return false;             // Normal display (not inverted)
    if (!sendCommand2(0xD5, 0x80)) return false;      // Set display clock divide ratio/oscillator frequency
    if (!sendCommand2(0x8D, 0x14)) return false;        // enable charge pump regulator
    if (!clear()) return false;                       // zero GDDRAM while display is off — prevents garbage on first scan
    if (!sendCommand(0xAF)) return false;             // Display ON

    return true;
    //needs_update_ = true;
    //return update();
}

bool SSD1306_OLED::clear() {
    for (uint8_t page = 0; page < (HEIGHT / 8); ++page) {
        uint8_t header[] = { 0x00, (uint8_t)(0xB0 | page), 0x02, 0x10 };
        if (!I2C::write(address_, header, sizeof(header))) return false;

        uint8_t buf[1 + WIDTH];
        buf[0] = 0x40;
        for (uint8_t i = 1; i <= WIDTH; ++i) buf[i] = 0x00;
        if (!I2C::write(address_, buf, sizeof(buf))) return false;
    }
    return true;
}

void SSD1306_OLED::setTime(Timekeeping::Time time) {
    if (time.hours != time_.hours || time.minutes != time_.minutes) {
        time_ = time;
        needs_update_ = true;
    }
}

// Send framebuffer to display (writes page by page)
bool SSD1306_OLED::update() {
    if (!needs_update_ &&
        time_.hours == last_displayed_time_.hours &&
        time_.minutes == last_displayed_time_.minutes) {
        return true; // Nothing to update
    }

    // Determine which digits to show
    uint8_t hour_tens_digit = (time_.hours / 10);
    if (time_.hours < 10) hour_tens_digit = 0;
    uint8_t hour_ones_digit = time_.hours % 10;
    uint8_t minute_tens_digit = time_.minutes / 10;
    uint8_t minute_ones_digit = time_.minutes % 10;

    // Get the canvas for each digit
    const oled_canvas* h10_c = get_canvas_for_digit(hour_tens_digit);
    const oled_canvas* h1_c = get_canvas_for_digit(hour_ones_digit);
    const oled_canvas* m10_c = get_canvas_for_digit(minute_tens_digit);
    const oled_canvas* m1_c = get_canvas_for_digit(minute_ones_digit);

    // Read canvas properties from PROGMEM once
    uint8_t h10_c_height = pgm_read_byte(&h10_c->height);
    const uint8_t* h10_c_data = (const uint8_t*)pgm_read_ptr(&h10_c->data);

    uint8_t h1_c_height = pgm_read_byte(&h1_c->height);
    const uint8_t* h1_c_data = (const uint8_t*)pgm_read_ptr(&h1_c->data);

    uint8_t m10_c_height = pgm_read_byte(&m10_c->height);
    const uint8_t* m10_c_data = (const uint8_t*)pgm_read_ptr(&m10_c->data);

    uint8_t m1_c_height = pgm_read_byte(&m1_c->height);
    const uint8_t* m1_c_data = (const uint8_t*)pgm_read_ptr(&m1_c->data);
    
    uint8_t colon_height = pgm_read_byte(&colon_char_colon.height);
    const uint8_t* colon_data = (const uint8_t*)pgm_read_ptr(&colon_char_colon.data);


    // For each page (8 pages for 64px tall)
    for (uint8_t page = 0; page < (HEIGHT / 8); ++page) {
        uint8_t header[] = {
            0x00, // control byte: command
            (uint8_t)(0xB0 | page), // Set page address
            0x2, // Set lower column start address, screen is shifted left by 2px, this fixes it
            0x10  // Set higher column start address
        };
        if (!I2C::write(address_, header, sizeof(header))) return false;

        uint8_t sendBuf[1 + WIDTH];
        sendBuf[0] = 0x40; // data control byte

        // Render the page content into the buffer on-the-fly
        for (uint8_t x = 0; x < WIDTH; ++x) {
            uint8_t data_byte = 0;
            if (x >= 0 && x < 28) { // Hour tens
                data_byte = pgm_read_byte(h10_c_data + x * (h10_c_height / 8) + page);
            } else if (x >= 28 && x < 56) { // Hour ones
                data_byte = pgm_read_byte(h1_c_data + (x-28) * (h1_c_height / 8) + page);
            } else if (x >= 56 && x < 72) { // Colon
                data_byte = pgm_read_byte(colon_data + (x-56) * (colon_height / 8) + page);
            } else if (x >= 72 && x < 100) { // Minute tens
                data_byte = pgm_read_byte(m10_c_data + (x-72) * (m10_c_height / 8) + page);
            } else if (x >= 100 && x < 128) { // Minute ones
                data_byte = pgm_read_byte(m1_c_data + (x-100) * (m1_c_height / 8) + page);
            }
            sendBuf[1 + x] = data_byte;
        }

        if (!I2C::write(address_, sendBuf, sizeof(sendBuf))) return false;
    }

    last_displayed_time_ = time_;
    needs_update_ = false;

    return true;
}

const oled_canvas* SSD1306_OLED::get_canvas_for_digit(uint8_t digit) {
    static const oled_canvas* const number_canvases[] PROGMEM = {
        &number_num_0, &number_num_1, &number_num_2, &number_num_3, &number_num_4,
        &number_num_5, &number_num_6, &number_num_7, &number_num_8, &number_num_9
    };
    if (digit > 9) digit = 0; // Should not happen, but safeguard
    return (const oled_canvas*)pgm_read_ptr(&number_canvases[digit]);
}


bool SSD1306_OLED::setContrast(uint8_t contrast) {
    return sendCommand2(0x81, contrast);
}

bool SSD1306_OLED::power(bool on) {
    uint8_t cmd = on ? 0xAF : 0xAE;
    if (on) {
        needs_update_ = true;
    }
    return sendCommand(cmd);
}

bool SSD1306_OLED::sendCommand(uint8_t cmd) {
    uint8_t data[2] = { COMMAND_BYTE, cmd };
    return I2C::write(address_, data, 2);
}

bool SSD1306_OLED::sendCommand2(uint8_t cmd, uint8_t arg) {
    uint8_t data[3] = { COMMAND_BYTE, cmd, arg };
    return I2C::write(address_, data, 3);
}
