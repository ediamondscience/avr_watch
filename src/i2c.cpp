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