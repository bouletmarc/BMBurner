byte VerisonBytes[] = {
  33, 1, 0
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
#define A15     A5
#define A10     13

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
byte buffer[BUFFERSIZE];

//command buffer for parsing commands
#define COMMANDSIZE 3
byte cmdbuf[COMMANDSIZE];

//###############################################################
//###############################################################
//###############################################################

void setup() {
  //define the shiftOut Pins as output
  pinMode(DS, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
 
  //define the boost pins as output
  // take care that they are LOW
  digitalWrite(VH, LOW);
  pinMode(VH, OUTPUT);
  digitalWrite(VPP, LOW);
  pinMode(VPP, OUTPUT);

  //define the EEPROM Pins as output
  // take care that they are HIGH
  digitalWrite(OE, HIGH);
  pinMode(OE, OUTPUT);
  digitalWrite(CE, HIGH);
  pinMode(CE, OUTPUT);
  digitalWrite(A15, HIGH);
  pinMode(A15, OUTPUT);
  digitalWrite(A10, HIGH);
  pinMode(A10, OUTPUT);

  //set speed of serial connection
  Serial.begin(115200);
  //Serial.begin(460800);
}

void loop() {
  readCommand();
  if (cmdbuf[0] == 'V') Serial.write(VerisonBytes, 3);
  if (cmdbuf[0] == 'R') Read();
  if (cmdbuf[0] == 'W') Write();
  if (cmdbuf[0] == 'E') EraseSST();
}


//###############################################################
// Commands functions
//###############################################################

void readCommand() {
  for(int i=0; i< COMMANDSIZE;i++) cmdbuf[i] = 0;
  int idx = 0;
  
  do {
    if(Serial.available()) cmdbuf[idx++] = Serial.read();
  }
  while (idx < (COMMANDSIZE));
}

void Read() {
  long addr = ((long) cmdbuf[1] * 256) + (long) cmdbuf[2];
  read_start();
  for (int x = 0; x < BUFFERSIZE; ++x) {
    buffer[x] = read_byte(addr+x);
    delayMicroseconds(100);
  }
  read_end();
  Serial.write(buffer, sizeof(buffer));
}

void Write() {
  int LoopingIndex = 0;
  while(LoopingIndex < BUFFERSIZE) {
    if(Serial.available()) buffer[LoopingIndex++] = Serial.read();
  }
  long addr = ((long) cmdbuf[1] * 256) + (long) cmdbuf[2];
  write_start();
  for (int x = 0; x < BUFFERSIZE; ++x) {
    fast_write(addr + x, buffer[x]);
  }
  write_end();
  Serial.write(0x4F);
}

void EraseSST() {
  set_ce(HIGH);
  set_oe(HIGH);
  set_vh(HIGH);
  set_vpp(HIGH);
  delay(1);
  
  //erase pulse
  set_ce(LOW);
  delay(450);
  set_ce(HIGH);
  delayMicroseconds(1);

  //Turning Off
  set_vh(LOW);
  set_vpp(LOW);
  delayMicroseconds(1);
  
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
}

void read_end() {
  //disable output
  set_oe(HIGH);
  //disable chip select
  set_ce(HIGH);
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

  //programming pulse
  set_ce(LOW);
  delayMicroseconds(20); // for 27SF512
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
  digitalWrite(A15, (address & 32768)?HIGH:LOW);
  
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
    
    //On/Off
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

void set_vpp (byte state)
{
  digitalWrite(VPP, state);
}

void set_vh (byte state)
{
  digitalWrite(VH, state);
}

//###############################################################
// FAST SWIFT functions (OLD FUNCTION)
//###############################################################

/*byte read_data_bus()
{
 //return (PIND >> 2) | ((PINB & 0x3) << 6);
  
 return ((digitalRead(D7) << 7) |
    (digitalRead(D6) << 6) |
    (digitalRead(D5) << 5) |
    (digitalRead(D4) << 4) |
    (digitalRead(D3) << 3) |
    (digitalRead(D2) << 2) |
    (digitalRead(D1) << 1) |
    digitalRead(D0));
}*/

/*inline void write_data_bus(byte data)
{
  //2 bits belong to PORTB and have to be set separtely
  PORTB = (PORTB & 0xF8) | (data >> 6);
  //bit 0 to 6 belong to bit 2 to 8 of PORTD
  PORTD = data << 2;

   //2 bits belong to PORTB and have to be set separtely 
  //digitalWrite(D6, (data >> 6) & 0x01);
  //digitalWrite(D7, (data >> 7) & 0x01);
  //bit 0 to 6 belong to bit 2 to 8 of PORTD
  //PORTD = data << 2;
}*/

//#define FAST_SHIFT(data) { \
  /*shift out the top bit of the byte*/ \
//  if (data & 0x80) \
//    bitSet(STROBE_PORT,STROBE_DS); \
//  else \
//    bitClear(STROBE_PORT,STROBE_DS); \
  /*shift data left so next bit is ready*/ \
//  data <<= 1; \
  /*register shifts bits on upstroke of clock pin*/ \
//  bitSet(STROBE_PORT,STROBE_CLOCK); \
//  bitClear(STROBE_PORT,STROBE_CLOCK); \
//}

/*inline void set_address_bus(unsigned int address)
{
  byte hi, low;
  hi = (address >> 8) & 0x3F;
  hi |= (address >> 9) & 0x40;
  
  //the 27x512 doesn't use WE, instead it's bit A14
  if (address & 0x4000)
      bitSet(STROBE_PORT, STROBE_WE);
  else
      bitClear(STROBE_PORT, STROBE_WE);
  //digitalWrite(WE, address & 0x4000 ? HIGH : LOW);

  //get low - byte of 16 bit address
  low = address & 0xff;

  //shift out highbyte using macro for speed
  FAST_SHIFT(hi); FAST_SHIFT(hi); FAST_SHIFT(hi); FAST_SHIFT(hi);
  FAST_SHIFT(hi); FAST_SHIFT(hi); FAST_SHIFT(hi); FAST_SHIFT(hi);
  //shift out lowbyte
  FAST_SHIFT(low); FAST_SHIFT(low); FAST_SHIFT(low); FAST_SHIFT(low);
  FAST_SHIFT(low); FAST_SHIFT(low); FAST_SHIFT(low); FAST_SHIFT(low);
  
  //strobe latch line
  bitSet(STROBE_PORT,STROBE_LATCH);
  bitClear(STROBE_PORT,STROBE_LATCH);

  delayMicroseconds(1);
}*/
