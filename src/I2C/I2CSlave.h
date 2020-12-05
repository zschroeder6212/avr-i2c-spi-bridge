#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <avr/interrupt.h>
#include <stdint.h>

#define I2C_CONTINUE  0
#define I2C_END       1

void I2C_init(uint8_t address);
void I2C_stop(void);
void I2C_setCallbacks(void (*recv)(uint8_t, uint8_t), void (*req)());

inline void __attribute__((always_inline)) I2C_transmitByte(uint8_t data)
{
  TWDR = data;
}

ISR(TWI_vect);

#endif
