// Definitions for eeprom addr, data lines
const byte eeprom_addr_pins[] = {22,24,26,28,30,32,34,36,38,40,42,44,46,48,50};
const byte eeprom_data_pins[] = {23,25,27,29,31,33,35,37}; // Important additional pins for eeprom programming
#define _CE 9 //chip enable active low
#define _OE 8 //output enable active low
#define _WE 7 //write enable active low

// Various Writing & Reading related Constants
#define MAX_ADDR 0x7FFF
#define MAX_PAGE_WRITE_SIZE 64

// Serial Commands for user:
#define SERIAL_READ_EEPROM 0x72 
#define SERIAL_WRITE_EEPROM 0x77 
#define SERIAL_CLEAR_EEPROM 0x63
#define SERIAL_TEST_EEPROM 116
#define SERIAL_BLOCK_EEPROM 'b'


#define SET_WE() (PORTH |= (1 << 4))
#define CLR_WE() (PORTH &= ~(1 << 4))

#define SET_OE() (PORTH |= (1 << 5))
#define CLR_OE() (PORTH &= ~(1 <<5))

#define SET_CE() (PORTH |= (1 <<6))
#define CLR_CE() (PORTH &= (~(1 <<6)))

// declarations of functions
void parse_serial();
unsigned char read_at_addr(unsigned int addr);
void short_delay();
void write_at_addr(unsigned int, unsigned char);
char get_bit(unsigned int, unsigned char);
void page_write(unsigned int *, unsigned char *, unsigned int);


//Quick Macros for CNTRL Pins

//global variables
unsigned char serial_buff; // buffer for serial commands

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
	digitalWrite(_OE,1);
	digitalWrite(_WE,1);
	digitalWrite(_CE,1);
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
	if (serial_buff == SERIAL_TEST_EEPROM)
		Serial.println("d");
	else if (serial_buff == SERIAL_READ_EEPROM)
	{
		Serial.println("Reading from eeprom");
		char output = read_at_addr(0x0002);
		Serial.println("finished reading");
		Serial.print(output, HEX);
		Serial.println(" value obtained");
		digitalWrite(LED_BUILTIN, output == 0x50 ? 1 : 0);
	}
	else if (serial_buff == SERIAL_WRITE_EEPROM)
	{
		Serial.println("writing to eeprom");
		write_at_addr(0x0002, 0x59);
		Serial.println("Finished Writing");
	}
	else if (serial_buff == SERIAL_CLEAR_EEPROM)
	{
		Serial.println("clearing eeprom");
	}
	else if (serial_buff == SERIAL_BLOCK_EEPROM)
	{
		Serial.println("block_writing eeprom");
		unsigned int addr[64];
		unsigned char data[64];
		unsigned char i_data = 0;
		unsigned char i;
		for (i=0; i<64; i++)
		{
			addr[i] = i_data;
			i_data = i_data != 255 ? i_data +1 : 0;
		}
			
		page_write(addr,data,64);
		Serial.println("finished writing block");
	}
}

// simple get bit function. Note the datatypes that limit the data and index size
char get_bit(unsigned int data, unsigned char index) { return index > 15 ? 0 : ((data & (1 << index)) >> index); }

//reads back eeprom data at the specified address (note address are capped to 15 as our chosen eeprom has that as max
unsigned char read_at_addr(unsigned int addr)
{
	if (addr < MAX_ADDR)
	{
		char i;
		char buff = 0;
		digitalWrite(_OE,1);
		digitalWrite(_WE,1);
		digitalWrite(_CE,0);

		for (i=0; i< 15; i++)
			digitalWrite(eeprom_addr_pins[i],get_bit(addr, i));
		for (i=0; i< 8; i++)
			pinMode(eeprom_data_pins[i], INPUT);
		digitalWrite(_OE, 0);
		short_delay();
		for (i=0; i<8; i++)
			buff |= digitalRead(eeprom_data_pins[i]) <<i;
		short_delay();
		short_delay();
		short_delay();
		digitalWrite(_OE,1);
		digitalWrite(_CE,1);
		short_delay();
		return buff;
	}
	return 0;
		
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
		unsigned char i;
		
		for (i=0; i< 8; i++)
			pinMode(eeprom_data_pins[i], OUTPUT);
		
		for (i=0; i< 15; i++)
		{
			digitalWrite(eeprom_addr_pins[i], get_bit(addr,i));
		}
		for (i=0; i<8; i++)
		{
			digitalWrite(eeprom_data_pins[i], get_bit(data, i));
		}

		digitalWrite(_OE, 1);
		digitalWrite(_CE, 0);
		digitalWrite(_WE, 0);
		short_delay();
		short_delay();
		short_delay();
		
		digitalWrite(_CE, 1);
		digitalWrite(_WE, 1);
		short_delay();
	}
}


// Follows waveform for page writes roughly. Note that the size has to be less than or equal to 64 to work properly.
void page_write(unsigned int * address_arr, unsigned char * data_arr, unsigned int size)
{
	if (size <= 64)
	{
		char i,j;
		digitalWrite(_OE,1);
		digitalWrite(_CE,1);
		digitalWrite(_WE,1);
		
		for (i=0; i<8; i++)
			pinMode(eeprom_data_pins[i], OUTPUT);
		unsigned long init_time = millis();
		for (i=0; i<size; i++)
		{
			for (j=0; j<15; j++)
				digitalWrite(eeprom_addr_pins[j], get_bit(address_arr[i],j));
			for (j=0; j<8; j++)
				digitalWrite(eeprom_data_pins[j], get_bit(data_arr[i], j));
			short_delay();
			digitalWrite(_CE,0);
			digitalWrite(_WE,0);
			short_delay();
			short_delay();
			digitalWrite(_CE,1);
			digitalWrite(_WE,1);
			short_delay();
		}
		unsigned long end_time = millis();
		Serial.print("Time difference (millis): ");
		Serial.println(end_time-init_time);
		digitalWrite(_CE,1);
		digitalWrite(_WE,1);
		short_delay();
	}

}
