# BMBurner
BMBurner is a Arduino based EPROM Programmer that can Read/Write/Verify Chips (27C256, SST 27SF512, W27E512, etc...)

# [BUY BMBurner][]

![alt tag](https://raw.githubusercontent.com/bouletmarc/BMBurner/master/Wiring/BMBurner_V2.3_Top.jpg)

![alt tag](https://raw.githubusercontent.com/bouletmarc/BMBurner/master/Wiring/BMBurner_V2.3_Bottom.jpg)

# Downloads Links :
- [Windows Tool (BMBurner.exe)][] to Read/Write/Verify Chips
- [Firmware Updating Tool][] to Update the BMBurner Firmware

# Downloads Drivers Links :
- Driver for FTDI Chipset (BMBurner V1.1 and Moates) : [FTDI Driver][]
- Driver for CH340 Chipset (BMBurner V1.0, BMBurner V2.X) : [CH340 Driver][]

# How to Update the BMBurner Firmware (V2.X ONLY):
- Download and Install the [Firmware Updating Tool][] if not already done
1. Open the [Firmware Updating Tool][]
2. Select the COM Port of your BMBurner (and the baudrate, default is 921600)
3. Clic on 'Connect Device' button to connect to your BMBurner
4. Clic on the 'Update Firmware' button!

# Compatible Chips List (for BMBurner V2.X):
- 27C256 (Read-Only)
- 27C512 (Read-Only)
- 27SF256 (Read-Only)
- 27SF512
- 27SF512 (Fake with 70ns access/write time)
- 29C256
- W27C512
- W27E512
- DS1230
- 61256/61C256
- 62256/62C256

# BMBurner Windows Tool Functions :

*** Image may differ from the updated tool ***

![alt tag](https://github.com/bouletmarc/BMBurner/blob/master/Wiring/Arduino_Windows_GUI.png)

- Load File to Buffer
- Save Buffer to File
- Read Chip
- Program Chip
- Verify Chip
- Blank Check
- Erase Chip
- Edit Reading/Writing Chip addressing
- Edit Reading/Writing Buffer addressing
- Much Mores...

# Serial Protocol Functions (for developpers) :

| Command | Receice Buffer | Description
| --- | --- | --- |
| 'C'+R+'S'+'MSB'+'LSB'+'CS'| "B1"+"B2"+"...BX"+"CS" | Reads 'S' bytes(S=Size, S=0 for 256bytes) from the EEPROM at address 'MSB'+'LSB' (C=Chip, C=2 for 27C256, C=5 for 27SF512 SST) |
| 'C'+W+'S'+'MSB'+'LSB'+'B1'+'B2'+'...BX'+'CS'|"O"(0x4F) | Writes 'S' bytes(S=Size, S=0 for 256bytes) from the EEPROM at address 'MSB'+'LSB' (C=Chip, C=2 for 27C256, C=5 for 27SF512 SST) |
| 5+E+'CS' | "O"(0x4F) | Erase all the data on Chip Type 5 (SST) |
| V+'CS' (V+V) | "B1"+"B2" + "B3" | Prints the version bytes of the BMBurner PCB Board for Moates Compatibility(Ex:V5.1.0) |
| F+'CS' (F+F) | "B1"+"B2" | Prints the firmware bytes of the BMBurner PCB Board(Ex:V2.0) |
| G+'CS' (G+G) | "B1"+"B2" | Prints the BMBurner Starting baudrate (Ex:'B1=1,baud 921.6k', 'B1=2,baud 9600', 'B1=3,baud 115200') |
| S+0+S | "O"(0x4F) | Switch the internal baudrate on the BMBurner to 921600 for Moates Compatibility |
| S+7+S | "O"(0x4F) | Switch the internal baudrate on the BMBurner to 115600 for Moates Compatibility |
| A+'N' | "O"(0x4F) | Change the chip AccessTime to 'N'ns (Ex:N=70 for 70ns AccessTime) |

| Command | Descriptor
| --- | --- |
| 'N' | 'N' equal the AccessTime in ns (Ex:N=70 for 70ns AccessTime) |
| 'MSB' | 'MSB' equal to the Most Signifiant Byte for the EEPROM Address location |
| 'LSB' | 'LSB' equal to the Least Signifiant Byte for the EEPROM Address location |
| 'C' | 'C' equel the Chip Type (C=2 for 27C256, C=5 for 27SF512 SST, etc..) |
| 'S' | 'S' equal the Size/Amount of bytes to proceed (S=0 is for 256bytes else S equal the amount of byte, S=1 for 1byte) |
| 'CS' | 'CS' equal to the checksum of the Commands we are sending in 1Byte form |
| ... | Any Command that isn't within 'Brackets' are the actual Char/String to send (ex: V is for sending the Char V |

# Donations :

You can donate to me using [This Link][].

# Pictures BMBurner V1.1 :

![alt tag](https://raw.githubusercontent.com/bouletmarc/BMBurner/master/Wiring/BMBurner%20V1.1_LOTS.JPG)

# Pictures BMBurner V1.0 :

![alt tag](https://raw.githubusercontent.com/bouletmarc/BMBurner/master/Wiring/BMBurner%20V1.0_LOTS.JPG)

[This Link]: <https://www.paypal.me/bouletmarc>
[Project Files]: <https://github.com/bouletmarc/BMBurner/archive/master.zip>
[Windows Tool (BMBurner.exe)]: <https://github.com/bouletmarc/BMBurner/raw/master/BMBurner.exe>
[Arduino Software]: <https://www.arduino.cc/en/main/software>
[CH340 Driver]: <https://sparks.gogo.co.nz/assets/_site_/downloads/CH34x_Install_Windows_v3_4.zip>
[FTDI Driver]: <http://www.ftdichip.com/Drivers/CDM/CDM21228_Setup.zip>
[MiniCore]: <https://github.com/MCUdude/MiniCore>
[BUY BMBurner]: <https://bmdevs.fwscheckout.com/>
[Firmware Updating Tool]:<https://github.com/bouletmarc/BMDevsFirmwareUpdater>

