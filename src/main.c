#define F_CPU 16000000
#include "I2C/I2CSlave.h"
#include <avr/io.h>
#include <avr/delay.h>


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

volatile uint8_t received_index = 0;
volatile uint8_t received_size = 0;

volatile uint8_t transmit_index = 0;

volatile uint8_t mode = MODE_COMMAND;

volatile uint8_t CS = 0;

void SPI_init()
{
    /* set CS, MOSI and SCK to output */
    SPI_DDR |= (1 << PB2) | (1 << MOSI) | (1 << SCK);
    CS_DDR |= 0x0f;

    /* set all CS pins high */
    CS_PORT |= 0x0f;

    /* enable SPI, set as master, and clock to fosc/16 */
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

uint8_t SPI_rw(uint8_t data)
{
    /* transmit data */
    SPDR = data;

    /* Wait for reception complete */
    while(!(SPSR & (1 << SPIF)));

    /* return Data Register */
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
    {
      if(!stop && received_index < BUFFER_SIZE)
      { 
        buffer[received_index++] = SPI_rw(received_data);
      }else if(!stop && received_index >= BUFFER_SIZE){
        received_index = 0;
        buffer[received_index++] = SPI_rw(received_data);
      }else if(stop){
        CS_PORT |= 0x0f;
        received_index = 0;
      }
      break;
    }
    case MODE_CONFIGURE:
    {
      if(!stop)
      {
        uint8_t order = (received_data>>5)&0x01;
        uint8_t cpol = (received_data>>3)&0x01;
        uint8_t cpha = (received_data>>2)&0x01;
        SPCR = (order<<DORD)|(cpol<<CPOL)|(cpha<<CPHA);
      }
      break;
    }
  }

  if(stop)
  {
    mode = MODE_COMMAND;
  }
}

void I2C_requested()
{
  if(transmit_index >= BUFFER_SIZE)
  {
    transmit_index = 0;
    I2C_transmitByte(buffer[transmit_index++]);
  }else {
    I2C_transmitByte(buffer[transmit_index++]);
  }
}

void setup()
{
  /* init SPI as master */
  SPI_init();

  /* set received/requested callbacks */
  I2C_setCallbacks(I2C_received, I2C_requested);

  /* init I2C */
  I2C_init(I2C_ADDR);
}

int main()
{
  setup();
  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    buffer[i] = 0;
  }
  while(1);
}