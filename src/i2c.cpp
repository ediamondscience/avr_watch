// File: /home/ded/Projects/tinkermill/tinkermill_classes/eletronics/avr-watch/AVR-watch/src/i2c.cpp
//
// Simple bit-banged I2C master for ATtiny85 (SDA = PB0, SCL = PB2).
// Lightweight, blocking, ~100kHz (depends on _delay_us timing and F_CPU).
//
// Usage:
//   I2C::begin();
//   I2C::writeRegister(0x50, 0x00, 0x42);    // write 0x42 to reg 0x00 at addr 0x50
//   uint8_t v; I2C::readRegister(0x50, 0x00, v); // read back
//
// Notes:
// - Requires external pull-ups on SDA and SCL lines.
// - Keeps API small and synchronous.

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

class I2C {
public:
    // Public API
    static void begin();
    static bool write(uint8_t addr7, const uint8_t *data, uint8_t len);
    static bool read(uint8_t addr7, uint8_t *buf, uint8_t len);
    static bool writeRegister(uint8_t addr7, uint8_t reg, uint8_t val);
    static bool readRegister(uint8_t addr7, uint8_t reg, uint8_t &val);

private:
    // Pin definitions for ATtiny85
    static constexpr uint8_t SDA_b = PB0;
    static constexpr uint8_t SCL_b = PB2;

    // Low-level helpers
    static inline void sda_low()   { DDRB  |=  (1 << SDA_b); PORTB &= ~(1 << SDA_b); }
    static inline void sda_release(){ DDRB  &= ~(1 << SDA_b); PORTB &= ~(1 << SDA_b); } // input, no pull-up
    static inline uint8_t sda_read(){ return (PINB & (1 << SDA_b)) ? 1 : 0; }

    static inline void scl_low()   { DDRB  |=  (1 << SCL_b); PORTB &= ~(1 << SCL_b); }
    static inline void scl_release(){ DDRB  &= ~(1 << SCL_b); PORTB &= ~(1 << SCL_b); } // input, no pull-up

    static inline void i2c_delay() { _delay_us(5); } // ~100kHz clock when combined with code timing

    static void start_condition();
    static void stop_condition();
    static bool write_byte(uint8_t b);
    static uint8_t read_byte(bool ack);
};

// Public

void I2C::begin() {
    // Release both lines (inputs), assume external pull-ups
    sda_release();
    scl_release();
}

bool I2C::write(uint8_t addr7, const uint8_t *data, uint8_t len) {
    start_condition();
    if (!write_byte((addr7 << 1) | 0)) { stop_condition(); return false; } // write mode
    for (uint8_t i = 0; i < len; ++i) {
        if (!write_byte(data[i])) { stop_condition(); return false; }
    }
    stop_condition();
    return true;
}

bool I2C::read(uint8_t addr7, uint8_t *buf, uint8_t len) {
    start_condition();
    if (!write_byte((addr7 << 1) | 1)) { stop_condition(); return false; } // read mode
    for (uint8_t i = 0; i < len; ++i) {
        bool ack = (i + 1 < len);
        buf[i] = read_byte(ack);
    }
    stop_condition();
    return true;
}

bool I2C::writeRegister(uint8_t addr7, uint8_t reg, uint8_t val) {
    uint8_t d[2] = { reg, val };
    return write(addr7, d, 2);
}

bool I2C::readRegister(uint8_t addr7, uint8_t reg, uint8_t &val) {
    // Write register pointer, then repeated-start and read one byte
    start_condition();
    if (!write_byte((addr7 << 1) | 0)) { stop_condition(); return false; }
    if (!write_byte(reg)) { stop_condition(); return false; }
    // repeated start
    start_condition();
    if (!write_byte((addr7 << 1) | 1)) { stop_condition(); return false; }
    val = read_byte(false); // NACK after single byte
    stop_condition();
    return true;
}

// Private

void I2C::start_condition() {
    sda_release();
    scl_release();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

void I2C::stop_condition() {
    sda_low();
    i2c_delay();
    scl_release();
    i2c_delay();
    sda_release();
    i2c_delay();
}

bool I2C::write_byte(uint8_t b) {
    for (uint8_t i = 0; i < 8; ++i) {
        if (b & 0x80) sda_release(); else sda_low();
        b <<= 1;
        i2c_delay();
        scl_release();
        i2c_delay();
        scl_low();
        i2c_delay();
    }
    // ACK bit
    sda_release(); // release SDA for ACK
    i2c_delay();
    scl_release();
    i2c_delay();
    bool ack = (sda_read() == 0);
    scl_low();
    i2c_delay();
    return ack;
}

uint8_t I2C::read_byte(bool ack) {
    uint8_t b = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        b <<= 1;
        sda_release();
        i2c_delay();
        scl_release();
        i2c_delay();
        if (sda_read()) b |= 1;
        scl_low();
        i2c_delay();
    }
    // send ACK/NACK
    if (ack) sda_low(); else sda_release();
    i2c_delay();
    scl_release();
    i2c_delay();
    scl_low();
    i2c_delay();
    sda_release();
    return b;
}