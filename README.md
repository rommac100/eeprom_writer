# This is the branch for the host machine code for the eeprom programmer. 
Effectively, the code here will be trying to bitstream a recently generated assembled and linked binary file to a given Arduino (most likely a mega). Note that only Linux Serial libraries are compatible so only if demand of a need arises, additional crossplatform libraries will not be added.

# Compilation instructions:
Simple for now will add Makefile if necessary
```bash
gcc -Wall binary_eeprom_writer.c -o binary_eeprom_writer
```

# Running Instructions:
Effectively, there are two commandline arguments the binary file path, and the serial device path. And sense you have to use serial device make sure you have the proper permissions to do so as a normal user or execute the below command temporarily with root privileges.
```bash
./binary_eeprom_writer
```
