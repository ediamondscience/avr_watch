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

#include "i2c.h"

// Low-level helpers
inline void I2C::sda_low() {
    DDRB |= (1 << SDA_b); // Set SDA as output
    PORTB &= ~(1 << SDA_b); // Drive SDA low
}

inline void I2C::sda_release() {
    DDRB &= ~(1 << SDA_b); // Set SDA as input
    PORTB |= (1 << SDA_b); // internal pull-up
}

uint8_t I2C::sda_read() {
    return (PINB & (1 << SDA_b)) ? 1 : 0;
}

inline void I2C::scl_low() {
    DDRB |= (1 << SCL_b); // Set SCL as output
    PORTB &= ~(1 << SCL_b); // Drive SCL low
}

inline void I2C::scl_release() {
    DDRB &= ~(1 << SCL_b); // Set SCL as input
    PORTB |= (1 << SCL_b); // internal pull-up
}

inline void I2C::i2c_delay() {
    _delay_us(3); // ~100kHz with 1MHz clock, adjust as needed
}


// Public API

void I2C::begin() {
    // Turn off the USI port
    USICR = 0;
    // Re-enable digital input buffers for SDA (PB0/AIN0) and SCL (PB2/ADC1).
    // The Arduino framework may set DIDR0 bits to reduce ADC noise; if AIN0D or
    // ADC1D are set, PINB reads for those pins always return 0, breaking I2C.
    //DIDR0 &= ~((1 << AIN0D) | (1 << ADC1D));
    sda_release();
    scl_release();
}

bool I2C::write(uint8_t addr7, const uint8_t *data, uint8_t len) {
    start_condition();

    if (!write_byte(addr7 << 1)) { // Address with write bit
        stop_condition();
        return false;
    }

    for (uint8_t i = 0; i < len; ++i) {
        if (!write_byte(data[i])) {
            stop_condition();
            return false;
        }
    }

    stop_condition();
    return true;
}

bool I2C::read(uint8_t addr7, uint8_t *buf, uint8_t len) {
    start_condition();

    if (!write_byte((addr7 << 1) | 1)) { // Address with read bit
        stop_condition();
        return false;
    }

    for (uint8_t i = 0; i < len; ++i) {
        bool is_last_byte = (i == (len - 1));
        buf[i] = read_byte(!is_last_byte); // NACK on last byte
    }

    stop_condition();
    return true;
}

bool I2C::writeRegister(uint8_t addr7, uint8_t reg, uint8_t val) {
    start_condition();
    if (!write_byte(addr7 << 1)) { // address + write
        stop_condition();
        return false;
    }
    if (!write_byte(reg)) { // register
        stop_condition();
        return false;
    }
    if (!write_byte(val)) { // value
        stop_condition();
        return false;
    }
    stop_condition();
    return true;
}

bool I2C::readRegister(uint8_t addr7, uint8_t reg, uint8_t &val) {
    // Write register address
    start_condition();
    if (!write_byte(addr7 << 1)) { // address + write
        stop_condition();
        return false;
    }
    if (!write_byte(reg)) { // register
        stop_condition();
        return false;
    }

    // Read value
    start_condition(); // repeated start
    if (!write_byte((addr7 << 1) | 1)) { // address + read
        stop_condition();
        return false;
    }
    val = read_byte(false); // read one byte, send NACK
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
    while (!(PINB & (1 << SCL_b))) {}  // clock-stretch: wait for SCL to go HIGH
    i2c_delay();  // extra hold: SDA needs ~3.5V (ATtiny VIH), not just ~2V (LA threshold)
    sda_release();
    i2c_delay();
}

bool I2C::write_byte(uint8_t b) {
    for (uint8_t i = 0; i < 8; ++i) {
        if (b & 0x80) sda_release(); else sda_low();
        b <<= 1;
        scl_release();
        i2c_delay();
        scl_low();
        i2c_delay();
    }
    // ACK/NACK: release SDA so device can drive it, then wait for SCL to
    // actually read HIGH (handles slow pull-up rise time) before sampling.
    sda_release();
    i2c_delay();
    scl_release();
    while (!(PINB & (1 << SCL_b))) {}  // clock-stretch: wait for SCL to go HIGH
    i2c_delay();  // extra hold: SDA needs ~3.5V (ATtiny VIH), not just ~2V (LA threshold)
    bool ack = (sda_read() == 0);
    scl_low();
    i2c_delay();
    return ack;
}

uint8_t I2C::read_byte(bool ack) {
    uint8_t b = 0;
    sda_release(); // Make sure SDA is input
    for (uint8_t i = 0; i < 8; ++i) {
        b <<= 1;
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