#define F_CPU 8000000
#include "I2C/I2CSlave.h"

#define I2C_ADDR 0x28

#define SPI_DDR DDRB
#define MOSI    PB3
#define MISO    PB4
#define SCK     PB5

#define CS_DDR  DDRC
#define CS_PORT PORTC

#define MODE_COMMAND 0
#define MODE_TRANSMIT 1
#define MODE_CONFIGURE 2

#define BUFFER_SIZE 200

volatile uint8_t buffer[BUFFER_SIZE];

uint8_t received_index = 0;
uint8_t received_size = 0;

uint8_t transmit_index = 0;

uint8_t mode = MODE_COMMAND;

uint8_t CS = 0;

void SPI_init()
{
    // set CS, MOSI and SCK to output
    SPI_DDR |= (1 << PB2) | (1 << MOSI) | (1 << SCK);
    CS_DDR |= 0x0f;

    // set all CS pins high
    CS_PORT |= 0x0f;

    // enable SPI, set as master, and clock to fosc/16
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

uint8_t SPI_rw(uint8_t data)
{
    // transmit data
    SPDR = data;

    // Wait for reception complete
    while(!(SPSR & (1 << SPIF)));

    // return Data Register
    return SPDR;
}

void I2C_received(uint8_t received_data, uint8_t stop)
{
  switch(mode)
  {
    case MODE_COMMAND:
      if(!stop && received_data >= 0x01 && received_data < 0x0f)
      {
        CS = received_data;
        CS_PORT &= ~CS;
        mode = MODE_TRANSMIT;
      }else if(!stop && received_data == 0xf0)
      {
        mode = MODE_CONFIGURE;
      }
      break;
    case MODE_TRANSMIT:
      if(!stop && received_index < BUFFER_SIZE)
      {
        buffer[received_index++] = SPI_rw(received_data);
      }else if(stop){
        CS_PORT |= 0x0f;
      }
      break;
    case MODE_CONFIGURE:
      //TODO
      break;
  }

  if(stop)
  {
    mode = MODE_COMMAND;
  }
}

void I2C_requested()
{
  if(transmit_index >= received_index)
  {
    I2C_transmitByte(0);
    received_index = 0;
    transmit_index = 0;
  }else {
    I2C_transmitByte(buffer[transmit_index]);
    transmit_index++;
  }
}

void setup()
{
  // init SPI as master
  SPI_init();

  // set received/requested callbacks
  I2C_setCallbacks(I2C_received, I2C_requested);

  // init I2C
  I2C_init(I2C_ADDR);
}

int main()
{
  setup();

  while(1);
}