//####################################################################################################################################
//####################################################################################################################################
//####################################################################################################################################
//
// EEPROM Programmer - code for an Atmega328 - Written by BM Devs (Bouletmarc)
//
// This software presents a 921600-8N1 serial port. (Require 14.7456mhz clock)
// It is made to Read/Write Data from EEPROM directly with the Atmega328 over the USB Port!
// This program is made to read 28pins Chips such as 27c256, 27sf512, etc
// 
// For more informations visit : https://www.github.com/bouletmarc/BMBurner/
//
//
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// COMMANDS :                                                  |   OUTPUTS :                      |   DESC :
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// 2 + R + "MMSB" + "MSB" + "LSB" + "CS"                       | "B1" + "B2" + "...B256" + "CS"   |   Reads 256 bytes of data from the EEPROM at address "MSB" + "LSB"  ... On Chip Type  2
// 3 + W + "MMSB" + "MSB" + "LSB" + "B1" + "B2" + "..." + "CS" | "O" (0x4F)                       |   Writes 256 bytes of data to the EEPROM at address "MSB" + "LSB", This output 'O' (0x79) for telling 'OK'  ... On Chip Type 3
// 5 + E + "CS"                                                | "O" (0x4F)                       |   Erase all the data on Chip Type 5 (SST)
// V + CS  (V+V)                                               | "V1" + "V2" + "V3"               |   Prints the version bytes of the BMBurner PCB Board for Moates Compatibility (Ex: V5.1.0 = 0x05, 0x01, 0x00)
// V + F                                                       | "V1" + "V2"                      |   Prints the version bytes of the BMBurner Firmware aka Arduino Project Version (Ex: V1.1 = 0x01, 0x01)
//
//**MMSB is always 0, this is only for compatibility with moates softwares on baud 115200, since the all chips arent more than 64KB (0xFFFF) there are no MMSB**
//####################################################################################################################################
//####################################################################################################################################
//####################################################################################################################################

byte BoardVersionBytes[] = {
  5, 15, 70  //Burn2 Version = 5, 15, 70
};


byte FirmwareVersionBytes[] = {
  1, 3
};

//Define chips predefinied model numbers (2-5 is for moates compatibility, 4 is never ever as it should be used for 27SF040 which is not compatible with this project at all)
unsigned int chipType;
#define CHIP27C256    2   //INCLUDED : 29C256, 27SF256
#define CHIP27SF512   5 

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
#define VPP    11     //OE Pin = VPP    12v
#define VH     12     //Erase Pin (A9)  12v

// shiftOut part
#define DS     A0
#define LATCH  A1
#define CLOCK  A2

// define the IO lines for the eeprom control
#define OE     A3
#define CE     A4

// direct access to port
#define STROBE_PORT PORTC
#define STROBE_DS      0
#define STROBE_LATCH   1
#define STROBE_CLOCK   2

//a buffer for bytes to burn
#define BUFFERSIZE 256
byte Buffer[BUFFERSIZE + 1]; // +1 for checksum

//command buffer for parsing commands
int COMMANDSIZE = 2;    //Commands are set automatically when detecting commands
byte cmdbuf[262];

unsigned int Looping_Size = 0;

//Last Address (used for sending high/low output at only 1 specific location on the 74HC595
unsigned int Last_Address = 0;

//###############################################################
//###############################################################
//###############################################################

void setup() {
  //define the shiftOut Pins as output
  pinMode(DS, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
 
  //define the boost pins as output (take care that they are LOW)
  pinMode(VH, OUTPUT);
  pinMode(VPP, OUTPUT);
  digitalWrite(VH, LOW);
  digitalWrite(VPP, LOW);

  //define the EEPROM Pins as output (take care that they are HIGH)
  pinMode(OE, OUTPUT);
  pinMode(CE, OUTPUT);
  digitalWrite(OE, HIGH);
  digitalWrite(CE, HIGH);

  //set speed of serial connection
  Serial.begin(921600);
}

void loop() {
  readCommand();
  if (cmdbuf[0] == 'V' && cmdbuf[1] == 'V') Serial.write(BoardVersionBytes, 3);
  if (cmdbuf[0] == 'V' && cmdbuf[1] == 'F') Serial.write(FirmwareVersionBytes, 2);
  if (cmdbuf[1] == 'R') Read();
  if (cmdbuf[1] == 'W') Write();
  if (cmdbuf[0] == '5' && cmdbuf[1] == 'E') EraseSST();
  //if (cmdbuf[0] == 'S' && cmdbuf[1] == 0  && cmdbuf[2] == 'S') Serial.write(0x4F); //moates commands for 921.6k baudrates
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
    if (cmdbuf[1] == 'W' && cmdbuf[2] != 0)  COMMANDSIZE = cmdbuf[2] + 6;
    if (cmdbuf[0] == '5' && cmdbuf[1] == 'E') COMMANDSIZE = 3;
    //if (cmdbuf[0] == 'S' && cmdbuf[1] == 0) COMMANDSIZE = 3;  //moates commands for 921.6k baudrates
  }
  while (idx < (COMMANDSIZE));
}

void ChecksumThis() {
    byte num = 0;
    for (int i = 0; i < Looping_Size; i++) num = (byte) (num + Buffer[i]);
    Buffer[Looping_Size] = num;
}

//###############################################################
// Commands functions
//###############################################################

void Read() {
  //Get Parameters
  chipType = (int) cmdbuf[0] - 48;
  
  //Get Looping Size
  int ThisLooping = (int) cmdbuf[2];
  if (ThisLooping == 0) ThisLooping = 256;
  Looping_Size = ThisLooping;
  
  long addr = ((long) cmdbuf[3] * 256) + (long) cmdbuf[4];
  Last_Address = addr;
  
  //Read
  read_start();
  for (int x = 0; x < Looping_Size; ++x) Buffer[x] = read_byte(addr+x);
  read_end();

  //return Array+Checksum to say it passed
  ChecksumThis();
  Serial.write(Buffer, Looping_Size + 1);
}

void Write() {
  //Get Parameters
  chipType = (int) cmdbuf[0] - 48;
  
  //Get Looping Size
  int ThisLooping = (int) cmdbuf[2];
  if (ThisLooping == 0) ThisLooping = 256;
  Looping_Size = ThisLooping;
  
  long addr = ((long) cmdbuf[3] * 256) + (long) cmdbuf[4];
  Last_Address = addr;

  //Write
  write_start();
  for (int x = 0; x < Looping_Size; ++x) fast_write(addr + x, cmdbuf[x+5]);
  write_end();

  //return 0x4F (79) to say it passed
  Serial.write(0x4F);
}

void EraseSST() {
  //chipType = (int) cmdbuf[0] - 48;
  chipType = 5;
  set_ce(HIGH);
  set_oe(HIGH);
  set_vh(HIGH);
  set_vpp(HIGH);
  _delay_ms(1);
  
  //erase pulse
  set_ce(LOW);
  _delay_ms(120);
  set_ce(HIGH);
  _delay_us(1);

  //Turning Off
  set_vh(LOW);
  set_vpp(LOW);
  _delay_us(1);
  
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

  //Set VPP to Low/High (27C256)
  if (chipType == CHIP27C256) Set_Output_At(15, HIGH);
}

void read_end() {
  //disable output
  set_oe(HIGH);
  //disable chip select
  set_ce(HIGH);
  
  //Set VPP to Low/High (27C256)
  if (chipType == CHIP27C256)  Set_Output_At(15, LOW);
}  

inline byte read_byte(unsigned int address)
{
  set_address_bus(address);
  _delay_us(70);
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
  _delay_us(1);

  //programming pulse
  set_ce(LOW);
  _delay_us(20); //27SF512 should be 20ms
  set_ce(HIGH);
  _delay_us(1);
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
  
  ApplyShiftAt(hi, low);
}

inline void Set_Output_At(unsigned int Position, bool IsHigh)
{
  byte hi, low;
  hi = (Last_Address >> 8);
  low = Last_Address & 0xff;
  if (Position >= 8) {
    if (IsHigh) bitSet(hi, Position - 8);
    else  bitClear(hi, Position - 8);
  }
  else {
    if (IsHigh) bitSet(low, Position);
    else  bitClear(low, Position);
  }
  ApplyShiftAt(hi, low);
}

void ApplyShiftAt(byte hi, byte low)
{
  fastShiftOut(hi);
  fastShiftOut(low);
  
  //strobe latch line
  bitSet(STROBE_PORT,STROBE_LATCH);
  bitClear(STROBE_PORT,STROBE_LATCH);
  _delay_us(1);
}

void fastShiftOut(byte data) {
  //clear
  bitClear(STROBE_PORT,STROBE_DS);

  //Loop for the 8x outputs
  for (int i=7; i>=0; i--) {
    //clear clock pin
    bitClear(STROBE_PORT,STROBE_CLOCK);

    //Enable/Disable pin Output
    if (bitRead(data,i) == 1) bitSet(STROBE_PORT,STROBE_DS); 
    else  bitClear(STROBE_PORT,STROBE_DS);
    
    //register shifts bits on upstroke of clock pin  
    bitSet(STROBE_PORT,STROBE_CLOCK);
    //clear after shift to prevent bleed through
    bitClear(STROBE_PORT,STROBE_DS);
  }
  
  //stop shifting
  bitClear(STROBE_PORT,STROBE_CLOCK);
}

int GetAddress(unsigned int Position)
{
  int Address = 0;
  if (Position == 1) Address = 1;
  if (Position == 2) Address = 2;
  if (Position == 3) Address = 4;
  if (Position == 4) Address = 8;
  if (Position == 5) Address = 16;
  if (Position == 6) Address = 32;
  if (Position == 7) Address = 64;
  if (Position == 8) Address = 128;
  if (Position == 9) Address = 256;
  if (Position == 10) Address = 512;
  if (Position == 11) Address = 1024;
  if (Position == 12) Address = 2048;
  if (Position == 13) Address = 4096;
  if (Position == 14) Address = 8192;
  if (Position == 15) Address = 16384;
  if (Position == 16) Address = 32768;
  return Address;
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
