//####################################################################################################################################
//####################################################################################################################################
//####################################################################################################################################
//
// EEPROM Programmer - code for an Arduino Nano V3.0 && Arduino Uno (anys Atmega328) - Written by BM Devs (Bouletmarc)
//
// This software presents a 115200-8N1 serial port.
// It is made to Read/Write Data from EEPROM directly with the Arduino!
// This program is made to read 28pins Chips such as 27c256, 27sf512
// 
// For more informations visit : https://www.github.com/bouletmarc/BMBurner/
//
//
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// COMMANDS :                                                  |   OUTPUTS :                      |   DESC :
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// 2 + R + "MMSB" + "MSB" + "LSB" + "CS"                       | "B1" + "B2" + "...B256" + "CS"   |   Reads 256 bytes of data from the EEPROM at address "MSB" + "LSB"  ... Only for 27c256 Chip
// 5 + R + "MMSB" + "MSB" + "LSB" + "CS"                       | "B1" + "B2" + "...B256" + "CS"   |   Reads 256 bytes of data from the EEPROM at address "MSB" + "LSB"  ... Only for SST Chip
// 2 + W + "MMSB" + "MSB" + "LSB" + "B1" + "B2" + "..." + "CS" | "O" (0x4F)                       |   Writes 256 bytes of data to the EEPROM at address "MSB" + "LSB", This output 'O' (0x79) for telling 'OK'  ... Only for 27c256 Chip
// 5 + W + "MMSB" + "MSB" + "LSB" + "B1" + "B2" + "..." + "CS" | "O" (0x4F)                       |   Writes 256 bytes of data to the EEPROM at address "MSB" + "LSB", This output 'O' (0x79) for telling 'OK'  ... Only for SST Chip
// 5 + E + "CS"                                                | "O" (0x4F)                       |   Erase all the data on the SST Chip
// V + CS  (V+V)                                               | "V1" + "V2" + "V3"               |   Prints the version bytes of the BMBurner PCB Board for Moates Compatibility (Ex: V5.1.0 = 0x05, 0x01, 0x00)
// V + F                                                       | "V1" + "V2"                      |   Prints the version bytes of the BMBurner Firmware aka Arduino Project Version (Ex: V1.1 = 0x01, 0x01)
//
//**MMSB is always 0, this is only for compatibility with moates softwares on baud 115200, since the 27C256&27SF512 arent more than 64KB (0xFFFF) there are no MMSB**
//####################################################################################################################################
//####################################################################################################################################
//####################################################################################################################################

byte BoardVerisonBytes[] = {
  5, 1, 0
};

byte FirmwareVerisonBytes[] = {
  1, 1
};

// define the IO lines for the data - bus
#define D0 2
#define D1 3
#define D2 4
#define D3 5
#define D4 6
#define D5 7
#define D6 8
#define D7 10

// for high voltage programming supply 
#define VH     11     //Erase Pin (A9)  12v
#define VPP    12     //OE Pin = VPP    12v

// shiftOut part
#define DS     A0
#define LATCH  A1
#define CLOCK  A2

// define the IO lines for the eeprom control
#define CE     A3
#define OE     A4
#define A15VPP A5     //A15 on SST, VPP on 27C256
#define A10    13

// direct access to port
#define STROBE_PORT PORTC
#define STROBE_DS      0
#define STROBE_LATCH   1
#define STROBE_CLOCK   2
#define STROBE_CE      3
#define STROBE_OE      4
#define STROBE_WE      5

//a buffer for bytes to burn
#define BUFFERSIZE 256
byte buffer[BUFFERSIZE + 1]; // +1 for checksum

//command buffer for parsing commands
int COMMANDSIZE = 2;    //Commands are set automatically when detecting commands
byte cmdbuf[262];

#define CHIP27C256 50
#define CHIP27SF512 53
unsigned int chipType;

//###############################################################
//###############################################################
//###############################################################

void setup() {
  //define the shiftOut Pins as output
  pinMode(DS, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
 
  //define the boost pins as output (take care that they are LOW)
  digitalWrite(VH, LOW);
  pinMode(VH, OUTPUT);
  digitalWrite(VPP, LOW);
  pinMode(VPP, OUTPUT);

  //define the EEPROM Pins as output (take care that they are HIGH)
  digitalWrite(OE, HIGH);
  pinMode(OE, OUTPUT);
  digitalWrite(CE, HIGH);
  pinMode(CE, OUTPUT);
  digitalWrite(A15VPP, HIGH);
  pinMode(A15VPP, OUTPUT);
  digitalWrite(A10, HIGH);
  pinMode(A10, OUTPUT);

  //set speed of serial connection
  Serial.begin(115200);
}

void loop() {
  readCommand();
  if (cmdbuf[0] == 'V' && cmdbuf[1] == 'V') Serial.write(BoardVerisonBytes, 3);
  if (cmdbuf[0] == 'V' && cmdbuf[1] == 'F') Serial.write(FirmwareVerisonBytes, 2);
  if (cmdbuf[1] == 'R') Read();
  if (cmdbuf[1] == 'W') Write();
  if (cmdbuf[0] == '5' && cmdbuf[1] == 'E') EraseSST();
}

//###############################################################
// Serial Bytes functions
//###############################################################

void readCommand() {
  for(int i=0; i< COMMANDSIZE;i++) cmdbuf[i] = 0;
  int idx = 0;
  COMMANDSIZE = 2;
  
  do {
    if(Serial.available()) cmdbuf[idx++] = Serial.read();
    if (cmdbuf[1] == 'R') COMMANDSIZE = 6;
    if (cmdbuf[1] == 'W') COMMANDSIZE = 262;
    if (cmdbuf[0] == '5' && cmdbuf[1] == 'E') COMMANDSIZE = 3;
  }
  while (idx < (COMMANDSIZE));
}

void ChecksumThis() {
    byte num = 0;
    for (int i = 0; i < BUFFERSIZE; i++)
        num = (byte) (num + buffer[i]);
    buffer[256] = num;
}

//###############################################################
// Commands functions
//###############################################################

void Read() {
  //Get Parameters
  chipType = (int) cmdbuf[0];
  long addr = ((long) cmdbuf[3] * 256) + (long) cmdbuf[4];

  //Read
  read_start();
  for (int x = 0; x < BUFFERSIZE; ++x) {
    buffer[x] = read_byte(addr+x);
    delayMicroseconds(100);
  }
  read_end();

  //return Array+Checksum to say it passed
  ChecksumThis();
  Serial.write(buffer, sizeof(buffer));
}

void Write() {
  //Get Parameters
  chipType = (int) cmdbuf[0];
  long addr = ((long) cmdbuf[3] * 256) + (long) cmdbuf[4];

  //Write
  write_start();
  for (int x = 0; x < BUFFERSIZE; ++x) fast_write(addr + x, cmdbuf[x+5]);
  write_end();

  //return 0x4F (79) to say it passed
  Serial.write(0x4F);
}

void EraseSST() {
  chipType = (int) cmdbuf[0];
  set_ce(HIGH);
  set_oe(HIGH);
  set_vh(HIGH);
  set_vpp(HIGH);
  delay(1);
  
  //erase pulse
  set_ce(LOW);
  delay(350);
  set_ce(HIGH);
  delayMicroseconds(1);

  //Turning Off
  set_vh(LOW);
  set_vpp(LOW);
  delayMicroseconds(1);
  
  //return 0x4F (79) to say it passed
  Serial.write(0x4F);
}

//###############################################################
// COMMANDS SUBS functions
//###############################################################

void read_start() {
  data_bus_input();
  //enable chip select
  set_ce(LOW);
  //enable output
  set_oe(LOW);

  //Set VPP to High (27C256)
  switch (chipType) {
    case CHIP27C256:
      digitalWrite(A15VPP, HIGH);
      break;
    default:
      break;
  }
}

void read_end() {
  //disable output
  set_oe(HIGH);
  //disable chip select
  set_ce(HIGH);
  
  //Set VPP to Low (27C256)
  switch (chipType) {
    case CHIP27C256:
      digitalWrite(A15VPP, LOW);
      break;
    default:
      break;
  }
}  

inline byte read_byte(unsigned int address)
{
  set_address_bus(address);
  return read_data_bus();
}
 
void write_start() {
  //disable output
  set_oe(HIGH);
  set_vpp(HIGH);
  data_bus_output();
}

void write_end() {
  set_vpp(LOW);
  data_bus_input();
}

inline void fast_write(unsigned int address, byte data)
{
  set_address_bus(address);
  write_data_bus(data);
  delayMicroseconds(1);

  //programming pulse start
  set_ce(LOW);

  //programming pulse middle
  switch (chipType) {
    //case CHIP27C256:
      //set_we(LOW);
      //set_we(HIGH);
      //break;
    case CHIP27SF512:
      delayMicroseconds(20);
      break;
    default:
      break;
  }
  
  //programming pulse end
  set_ce(HIGH);
  delayMicroseconds(1);
}

//###############################################################
// DATA BUS functions
//###############################################################

void data_bus_input() {
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
}

void data_bus_output() {
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
}

byte read_data_bus()
{
 byte b = 0;
  if (digitalRead(D0) == HIGH) b |= 1;
  if (digitalRead(D1) == HIGH) b |= 2;
  if (digitalRead(D2) == HIGH) b |= 4;
  if (digitalRead(D3) == HIGH) b |= 8;
  if (digitalRead(D4) == HIGH) b |= 16;
  if (digitalRead(D5) == HIGH) b |= 32;
  if (digitalRead(D6) == HIGH) b |= 64;
  if (digitalRead(D7) == HIGH) b |= 128;
  return(b);
}

inline void write_data_bus(byte data)
{
  digitalWrite(D0, data & 1);
  digitalWrite(D1, data & 2);
  digitalWrite(D2, data & 4);
  digitalWrite(D3, data & 8);
  digitalWrite(D4, data & 16);
  digitalWrite(D5, data & 32);
  digitalWrite(D6, data & 64);
  digitalWrite(D7, data & 128);
}

//###############################################################
// FAST SWIFT functions
//###############################################################

inline void set_address_bus(unsigned int address)
{
  byte hi, low;
  hi = (address >> 8);
  low = address & 0xff;
  fastShiftOut(hi);
  fastShiftOut(low);
  digitalWrite(A10, (address & 1024)?HIGH:LOW );
  switch (chipType) {
  case CHIP27SF512:
      digitalWrite(A15VPP, (address & 32768)?HIGH:LOW);
      break;
    default:
      break;
  }
  
  //strobe latch line
  bitSet(STROBE_PORT,STROBE_LATCH);
  bitClear(STROBE_PORT,STROBE_LATCH);
  delayMicroseconds(1);
}

void fastShiftOut(byte data) {
  //clear
  bitClear(STROBE_PORT,STROBE_DS);
  
  for (int i=7; i>=0; i--)  {
    bitClear(STROBE_PORT,STROBE_CLOCK);
    if ( bitRead(data,i) == 1) {
      bitSet(STROBE_PORT,STROBE_DS);
    }
    else {      
      bitClear(STROBE_PORT,STROBE_DS);
    }
    
    //register shifts bits on upstroke of clock pin  
    bitSet(STROBE_PORT,STROBE_CLOCK);
    //clear after shift to prevent bleed through
    bitClear(STROBE_PORT,STROBE_DS);
  }
  
  //stop shifting
  bitClear(STROBE_PORT,STROBE_CLOCK);
}

//###############################################################
// PINS functions
//###############################################################

//**attention, this line is LOW - active**
inline void set_oe (byte state)
{
  digitalWrite(OE, state);
}
 
//**attention, this line is LOW - active**
inline void set_ce (byte state)
{
  digitalWrite(CE, state);
}

//Boost VPP 12V
void set_vpp (byte state)
{
  switch (chipType) {
  case CHIP27SF512:
    digitalWrite(VPP, state);
    break;
  default:
    break;
  }
}

//Boost Erase 12V
void set_vh (byte state)
{
  switch (chipType) {
  case CHIP27SF512:
    digitalWrite(VH, state);
    break;
  default:
    break;
  }
}
