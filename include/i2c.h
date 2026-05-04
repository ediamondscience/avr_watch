#ifndef I2C_H
#define I2C_H

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
    static constexpr uint8_t SDA_b = 0; // PB0
    static constexpr uint8_t SCL_b = 2; // PB2

    // Low-level helpers
    static inline void sda_low();
    static inline void sda_release();
    static inline uint8_t sda_read();

    static inline void scl_low();
    static inline void scl_release();

    static inline void i2c_delay();

    static void start_condition();
    static void stop_condition();
    static bool write_byte(uint8_t b);
    static uint8_t read_byte(bool ack);
};

#endif // I2C_H