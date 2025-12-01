// GME12864_OLED.cpp
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

#include <stdint.h>
#include <string.h>

class I2C {
public:
    // Provide your project's I2C write method here.
    // Return true on success, false on failure.
    bool write(uint8_t address, const uint8_t *data, size_t length);
};

class GME12864_OLED {
public:
    static constexpr uint8_t WIDTH  = 128;
    static constexpr uint8_t HEIGHT = 64;
    static constexpr uint16_t BUFFER_SIZE = (WIDTH * HEIGHT) / 8;

    GME12864_OLED(I2C &i2c, uint8_t address = 0x3C)
        : i2c_(i2c), address_(address) {
        clear();
    }

    // Initialize display (basic sequence; adapt for exact controller)
    bool init() {
        // A compact init sequence (SSD1306-like). Modify if your controller differs.
        const uint8_t cmds[] = {
            0xAE,             // Display OFF
            0xD5, 0x80,       // Set display clock divide ratio/oscillator frequency
            0xA8, 0x3F,       // Set multiplex ratio (1 to 64) => 0x3F = 64
            0xD3, 0x00,       // Set display offset
            0x40,             // Set start line = 0
            0x8D, 0x14,       // Charge pump (enable)
            0x20, 0x00,       // Memory addressing mode: horizontal
            0xA1,             // Segment remap
            0xC8,             // COM output scan direction
            0xDA, 0x12,       // COM pins hardware configuration
            0x81, 0xCF,       // Contrast
            0xD9, 0xF1,       // Pre-charge period
            0xDB, 0x40,       // VCOMH deselect level
            0xA4,             // Entire display ON resume
            0xA6,             // Normal display (not inverted)
            0xAF              // Display ON
        };
        if (!sendCommandBlock(cmds, sizeof(cmds))) return false;
        clear();
        return update();
    }

    bool clear() {
        memset(buffer_, 0x00, sizeof(buffer_));
        return true;
    }

    // Draw or clear a single pixel (x: 0..127, y: 0..63)
    void setPixel(uint8_t x, uint8_t y, bool on) {
        if (x >= WIDTH || y >= HEIGHT) return;
        uint16_t byteIndex = (y / 8) * WIDTH + x;
        uint8_t bit = 1u << (y & 7);
        if (on) buffer_[byteIndex] |= bit;
        else    buffer_[byteIndex] &= ~bit;
    }

    // Write an ASCII string at approximate position using a 5x7 font
    // (very small helper, not full-featured). Caller must implement mapping if needed.
    void drawChar5x7(uint8_t x, uint8_t y, char c, bool on);
    void drawString(uint8_t x, uint8_t y, const char *s);

    // Send framebuffer to display (writes page by page)
    bool update() {
        // For each page (8 pages for 64px tall)
        for (uint8_t page = 0; page < (HEIGHT / 8); ++page) {
            uint8_t header[] = {
                0x00, // control byte: command
                uint8_t(0xB0 | page), // Set page address
                0x00, // Set lower column start address
                0x10  // Set higher column start address
            };
            if (!i2c_.write(address_, header, sizeof(header))) return false;

            // Prepare a local buffer: first byte control (0x40 = data), then 128 bytes of data
            uint8_t sendBuf[1 + WIDTH];
            sendBuf[0] = 0x40; // data control byte
            memcpy(&sendBuf[1], &buffer_[page * WIDTH], WIDTH);

            if (!i2c_.write(address_, sendBuf, sizeof(sendBuf))) return false;
        }
        return true;
    }

    bool setContrast(uint8_t contrast) {
        uint8_t cmds[] = { 0x81, contrast };
        return sendCommandBlock(cmds, sizeof(cmds));
    }

    bool power(bool on) {
        uint8_t cmd = on ? 0xAF : 0xAE;
        return sendCommand(cmd);
    }

private:
    I2C &i2c_;
    uint8_t address_;
    uint8_t buffer_[BUFFER_SIZE];

    bool sendCommand(uint8_t cmd) {
        uint8_t data[2] = { 0x00, cmd }; // 0x00 = control byte for command
        return i2c_.write(address_, data, 2);
    }

    bool sendCommandBlock(const uint8_t *cmds, size_t len) {
        // For simplicity send as a sequence of [0x00, cmd] pairs.
        // Some controllers let you send many commands in one packet with a single 0x00 prefix,
        // adapt if your I2C implementation/controller prefers different packing.
        for (size_t i = 0; i < len; ++i) {
            if (!sendCommand(cmds[i])) return false;
        }
        return true;
    }
};

// --- Minimal 5x7 font (only chars 32..127) ---
// Provide your own font table or expand as needed.
// For brevity this is a placeholder: implement proper font data to use drawChar5x7/drawString.
static const uint8_t font5x7_basic[96][5] = {
    // All zeros -> blank. User should fill this table with real 5-byte glyphs.
    // Example for space and '!' (if desired) can be inserted here.
};

void GME12864_OLED::drawChar5x7(uint8_t x, uint8_t y, char c, bool on) {
    if (c < 32 || c > 127) return;
    const uint8_t *glyph = font5x7_basic[c - 32];
    for (uint8_t col = 0; col < 5; ++col) {
        uint8_t colByte = glyph[col];
        for (uint8_t row = 0; row < 7; ++row) {
            bool pixelOn = (colByte >> row) & 1;
            setPixel(x + col, y + row, pixelOn ? on : !on);
        }
    }
}

void GME12864_OLED::drawString(uint8_t x, uint8_t y, const char *s) {
    while (*s) {
        drawChar5x7(x, y, *s, true);
        x += 6; // 5 pixels + 1 space
        ++s;
        if (x + 5 >= WIDTH) break;
    }
}