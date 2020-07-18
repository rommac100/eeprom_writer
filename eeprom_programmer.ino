// Definitions for eeprom addr, data lines
const byte eeprom_addr_pins[] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};
const byte eeprom_data_pins[] = {37,38,39,40,41,42,43,44};

// Important additional pins for eeprom programming
#define _CE 45 //chip enable active low
#define _OE 46 //output enable active low
#define _WE 47 //write enable active low

#define MAX_ADDR 0x7FFF


// Serial Commands for user:
#define SERIAL_READ_EEPROM 0x72 // char lowercase r
#define SERIAL_WRITE_EEPROM 0x77 // char lowercase w
#define SERIAL_CLEAR_EEPROM 0x63 // char lowercase c
#define SERIAL_TEST_EEPROM 0x74 // char lowercase - used for testing serial

// declarations of functions
void enable_read();
void enable_write();
void disable_chip();
void parse_serial();
unsigned char read_at_addr(unsigned int addr);
void short_delay();
void write_at_addr(unsigned int addr, unsigned char);

//global variables
unsigned char serial_buff; // buffer for serial commands
unsigned char _CE_state;
unsigned char _OE_state;
unsigned char _WE_state;


void setup() {
//initalize serial
	Serial.begin(9600);

	//assign pinModes for both data and addr, assumes data pins are input (can be changed later
	char i;
	for (i=0; i< 15; i++)
		pinMode(eeprom_addr_pins[i], OUTPUT);
	for (i=0; i<8; i++)
		pinMode(eeprom_data_pins[i], INPUT);

	// other pins for setup
	pinMode(_CE, OUTPUT);
	pinMode(_OE, OUTPUT);
	pinMode(_WE, OUTPUT);

	// Sets up for Default state of EEPROM, not enable, not write, not read
	digitalWrite(_CE, 1);
	digitalWrite(_OE, 1);
	digitalWrite(_WE, 1);
}

void loop() {
// only proceceed if there is something to be read from the buffer

	if (Serial.available() > 0)
	{
		serial_buff = Serial.read();
		parse_serial();
	}
}

// parses the serial input appropriately and calls the associated functions
void parse_serial()
{
	switch (serial_buff)
	{
		case SERIAL_READ_EEPROM:
		break;
		case SERIAL_WRITE_EEPROM:
		break;
		case SERIAL_CLEAR_EEPROM:
		break;
		case SERIAL_TEST_EEPROM:
			Serial.println("d");
		break;
	}
}

void disable_chip()
{
	digitalWrite(_CE, 1);
	digitalWrite(_OE, 1);
	digitalWrite(_WE, 1);

	_CE_state = 1;
	_OE_state = 1;
	_WE_state = 1;
}

void enable_read()
{
	if (_CE_state != 0)
	{
		digitalWrite(_CE, 0);
		_CE_state = 0;
	}
	if (_WE_state != 1)
	{
		digitalWrite(_WE, 1);
		_WE_state = 1;
	}
	if (_OE_state != 0)
	{
		digitalWrite(_OE, 0);
		_OE_state = 0;
	}
}

void enable_write()
{
	if (_OE_state != 1)
	{
		digitalWrite(_OE, 1);
		_OE_state = 1;
	}
	if (_CE_state != 0)
	{
		digitalWrite(_CE, 0);
		_CE_state = 0;
	}
	if (_WE_state != 0)
	{
		digitalWrite(_WE, 0);
		_WE_state = 0;
	}
}


//reads back eeprom data at the specified address (note address are capped to 15 as our chosen eeprom has that as max
unsigned char read_at_addr(unsigned int addr)
{
	
	char i;
	if (addr <= MAX_ADDR)
	enable_read();
	{
	for (i =0; i< 14; i++)
		digitalWrite(eeprom_addr_pins[i], bitRead(addr,i));
	short_delay();
	short_delay();	
	short_delay();
	// incase the pins for data have been set to write previously
	for (i=0; i<8; i++)
		pinMode(eeprom_data_pins[i], INPUT);
	char tmp_data = 0; 
	for (i=0; i<8; i++)
		tmp_data |= digitalRead(eeprom_data_pins[i]) << i;	
	return tmp_data;
	disable_chip();
	}
	return 0x00; // return null character if the address is too big
}
// in theory the nop instruction takes roughly 62.5ns on 16 mhz based arduino so this is the best resolution that can be given as delay
void short_delay()
{
  __asm__("nop\n\t"); 
}

//somewhat follows the recommend write waveform. Will need to be tested
void write_at_addr(unsigned int addr, unsigned char data)
{
	if (addr <= MAX_ADDR)
	{
		char i;
		enable_write();
		for (i =0; i< 14; i++)
			digitalWrite(eeprom_addr_pins[i], bitRead(addr,i));
		// incase the pins for data have been set to write previously
		for (i=0; i<8; i++)
			pinMode(eeprom_data_pins[i], OUTPUT);
		for (i=0; i<8; i++)
			digitalWrite(eeprom_data_pins[i], bitRead(data, i));
		
		short_delay();
		short_delay();
		disable_chip();
	}
}
