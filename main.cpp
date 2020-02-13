#include <Arduino.h>
#include <SPI.h>

#define CS_PIN PA4

#define SPIFLASH_WRITE_EN 0x06
#define SPIFLASH_WRITE_DISABLE 0x04
#define SPIFLASH_BLOCKERASE_4K 0x20
#define SPIFLASH_BLOCKERASE_32K 0x52
#define SPIFLASH_BLOCKERASE_64K 0xD8
#define SPIFLASH_CHIPERASE 0x60
#define SPIFLASH_STATUS_READ 0x05
#define SPIFLASH_STATUS_WRITE 0x01
#define SPIFLASH_ARRAY_READ 0x0B
#define SPIFLASH_ARRAY_READLOWFREQ 0x03
#define SPIFLASH_SLEEP 0xB9
#define SPIFLASH_WAKE 0xAB
#define SPIFLASH_BYTEPAGEPROGRAM 0x02
#define SPIFLASH_JEDECID 0x9F
#define SPIFLASH_MANUID 0x90
#define SPIFLASH_UNIQUEID 0x4B

#define CS_ON() digitalWrite(CS_PIN, LOW)
#define CS_OFF() digitalWrite(CS_PIN, HIGH)

#define SPIFLAS_ID 0xEF3012

uint8_t busy(){
  CS_ON();
  SPI.transfer(SPIFLASH_STATUS_READ);
  uint8_t status = SPI.transfer(0);
  CS_OFF();
  return status & 1;
}

void wait(){
  while(busy());
}
void command(uint8_t cmd, bool isWrite = false);
void command(uint8_t cmd, bool isWrite){
  if(isWrite){
    command(SPIFLASH_WRITE_EN);
    CS_OFF();
  }
  if(cmd != SPIFLASH_WAKE) wait();
  CS_ON();
  SPI.transfer(cmd);
}

uint8_t readByte(uint32_t addr) {
  command(SPIFLASH_ARRAY_READLOWFREQ);
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  uint8_t result = SPI.transfer(0);
  CS_OFF();
  return result;
}
void readBytes(uint32_t addr, void* buf, uint16_t len) {
  command(SPIFLASH_ARRAY_READ);
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  SPI.transfer(0); //"dont care"
  for (uint16_t i = 0; i < len; ++i)
    ((uint8_t*) buf)[i] = SPI.transfer(0);
  CS_OFF();
}
void writeByte(uint32_t addr, uint8_t byt) {
  command(SPIFLASH_BYTEPAGEPROGRAM, true);  // Byte/Page Program
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  SPI.transfer(byt);
  CS_OFF();
}

void writeBytes(uint32_t addr, const void* buf, uint16_t len) {
  uint16_t n;
  uint16_t maxBytes = 256-(addr%256);  // force the first set of bytes to stay within the first page
  uint16_t offset = 0;
  while (len>0)
  {
    n = (len<=maxBytes) ? len : maxBytes;
    command(SPIFLASH_BYTEPAGEPROGRAM, true);  // Byte/Page Program
    SPI.transfer(addr >> 16);
    SPI.transfer(addr >> 8);
    SPI.transfer(addr);
    
    for (uint16_t i = 0; i < n; i++)
      SPI.transfer(((uint8_t*) buf)[offset + i]);
    CS_OFF();
    
    addr+=n;  // adjust the addresses and remaining bytes by what we've just transferred.
    offset +=n;
    len -= n;
    maxBytes = 256;   // now we can do up to 256 bytes per loop
  }
}

void SPIFlash_init(){
  CS_OFF();
  command(SPIFLASH_WAKE);
  CS_OFF();
  command(SPIFLASH_JEDECID);
  Serial1.print(SPI.transfer(0),HEX);
  Serial1.print(SPI.transfer(0),HEX);
  Serial1.println(SPI.transfer(0),HEX);
  CS_OFF();
  command(SPIFLASH_STATUS_WRITE,true);
  SPI.transfer(0);
  CS_OFF();
}

void setup() {
  Serial1.begin(115200);
  delay(2000);
  pinMode(CS_PIN,OUTPUT);
  SPI.begin();
  SPI.beginTransaction(SPISettings(500000,MSBFIRST,SPI_MODE0));
  SPIFlash_init();
  command(SPIFLASH_CHIPERASE,true);
  CS_OFF();
  wait();
  Serial1.println("Setup complete...");
}

uint8_t step = 0;
bool read = false;
uint32_t address = 0x00;
void loop() {
  if(!read){
    writeByte(address++,step++);
    if(address == 256){
      step = 0;
      read = true;
      address = 0;
    }
  } 
  else if(read){
    Serial1.println(readByte(address++));
    delay(100);
    if(address == 256){
      address = 0;
      read = false;
      command(SPIFLASH_CHIPERASE,true);
      CS_OFF();
      wait();
    }   
  }
}

