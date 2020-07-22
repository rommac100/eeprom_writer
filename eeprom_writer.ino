// Definitions for eeprom addr, data lines
const byte eeprom_addr_pins[] = {22,24,26,28,30,32,34,36,38,40,42,44,46,48,50};
const byte eeprom_data_pins[] = {23,25,27,29,31,33,35,37}; // Important additional pins for eeprom programming
#define _CE 9 //chip enable active low
#define _OE 8 //output enable active low
#define _WE 7 //write enable active low

// Various Writing & Reading related Constants
#define MAX_ADDR 0x7FFF
#define MAX_PAGE_WRITE_SIZE 64

#define MAX_BLOCK_BYTE_TRANSFER 32

// Serial Commands for user:
#define SERIAL_READ_EEPROM 'r' 
#define SERIAL_WRITE_EEPROM 'w' 
#define SERIAL_CLEAR_EEPROM 'c'
#define SERIAL_TEST_EEPROM 't'
#define SERIAL_BLOCK_EEPROM 'l'
#define SERIAL_BINARY_EEPROM 'b'

// short 62.5ns delay
#define NOP __asm__ __volatile__ ("nop\n\t")

#define SET_WE() (PORTH |= (1 << 4))
#define CLR_WE() (PORTH &= ~(1 << 4))

#define SET_OE() (PORTH |= (1 << 5))
#define CLR_OE() (PORTH &= ~(1 <<5))

#define SET_CE() (PORTH |= (1 <<6))
#define CLR_CE() (PORTH &= (~(1 <<6)))

// declarations of functions
void parse_serial();
unsigned char read_at_addr(unsigned int addr);
void write_at_addr(unsigned int, unsigned char);
char get_bit(unsigned int, unsigned char);
void page_write(unsigned int *, unsigned char *, unsigned int);
byte* data_polling(unsigned int *, int);

union size{
	unsigned int data;
	unsigned char data_char[2];
};

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
void parse_serial() {
	if (serial_buff == SERIAL_TEST_EEPROM)
		Serial.println("d");
	else if (serial_buff == SERIAL_READ_EEPROM)
	{
		Serial.println("Reading from eeprom");
		char output = read_at_addr(0x0001);
		Serial.println("finished reading");
		Serial.print(output, HEX);
		Serial.println(" value obtained");
		digitalWrite(LED_BUILTIN, output == 0x50 ? 1 : 0);
	}
	else if (serial_buff == SERIAL_WRITE_EEPROM)
	{
		Serial.println("writing to eeprom");
		write_at_addr(0x0001, 'b');
		Serial.println("Finished Writing");
	}
	else if (serial_buff == SERIAL_CLEAR_EEPROM)
	{
		Serial.println("clearing eeprom");
	}
	else if (serial_buff == SERIAL_BLOCK_EEPROM)
	{
		Serial.println("block_writing eeprom");
		unsigned int addr[] = {0,1};
		unsigned char data[] = {1,3};
		
		page_write(addr,data,2);
		Serial.println("finished writing block");
	}
	
	/* Effectively, the procedure here is that that host machine sends a binary write command request (sending char 'b').
	  Then the client receives the size and parses it and resends it to the host. 
	  The Host compares the two values (inital and after transfer) it then sends a 'y' or 'n' to say whether or not to continue with the transfer.
	  It then begins to bit stream typically 32 bytes at a time untill the 32 bytes are no longer possible. The client is prepared for this change since it knows the total transfer size and adjust accordingly. 
	  for each 32 byte segment of transfer, the host machine must wait for ready flag from the machine to verify that it is ready for the next packet (prevent the input FIFO from overflowing)
	*/
	else if (serial_buff == SERIAL_BINARY_EEPROM)
	{
		// Effectively, the binary write will pull 32 bytes at max at time. This means if the binary isn't perfectly divisble. Then the last chuck of will contain the not 32 byte count.
		union size size_trans;
		unsigned char i, j;

		for (i=0; i< 2; i++)
		{
		        while(!Serial.available());
			size_trans.data_char[i] = Serial.read();
		}
		for (i=0; i<2; i++)
			Serial.write(size_trans.data_char[i]);
		while(!Serial.available()); // wait for the next byte transfer of verification.
		if (Serial.read() == 'y') // proceed with block writing
		{
			size_trans.data = 2;
			unsigned int block_count = size_trans.data/MAX_BLOCK_BYTE_TRANSFER; 
			unsigned char data_block[MAX_BLOCK_BYTE_TRANSFER];
			int addr_arr[MAX_BLOCK_BYTE_TRANSFER];
			for (i=0; i< block_count; i++)
			{
				for (j=0; j<MAX_BLOCK_BYTE_TRANSFER; j++)
					addr_arr[j] = block_count*i+j;
				Serial.write('r'); //send ready transfer to send the information
				for (j=0; j<MAX_BLOCK_BYTE_TRANSFER; j++)
				{
					while (!Serial.available());
					data_block[j] = Serial.read();
				}
				page_write(addr_arr, data_block, MAX_BLOCK_BYTE_TRANSFER);
			}
			unsigned int last_block = size_trans.data-32*block_count;
			if (last_block > 0)
			{
				for (i=0; i<last_block; i++)
				{
					addr_arr[i] = 32*block_count+i;
					Serial.print("Address: ");
					Serial.println(addr_arr[i]);
				}
				Serial.write('r');
				for (i=0; i<last_block; i++)
				{
					while (!Serial.available());
					data_block[i] = Serial.read();	
					Serial.print("data received");
					Serial.println(data_block[i]);
				}
				page_write(addr_arr, data_block, last_block);
			}
			Serial.println("finished transfer");
		}
	}
			
}


// simple get bit function. Note the datatypes that limit the data and index size
char get_bit(unsigned int data, unsigned char index) { return index > 15 ? 0 : ((data & (1 << index)) >> index); }

byte* data_polling(unsigned int * addr_arr, int size)
{
	//alloc polling data
	char * data_arr = (char *) malloc(sizeof(char)*size);
	char i;
	
	//checks to make sure none of the addresses exceed the max_address size, returns null if any addresses are too big.
	for (i =0; i< size; i++)
	{
		if (addr_arr[i] > MAX_ADDR)
		{
			free(data_arr);
			return NULL;
		}
	}
	
			


}

//reads back eeprom data at the specified address (note address are capped to 15 as our chosen eeprom has that as max
//Note this still has some problems with wrong bits being read. More tuning is needed.
unsigned char read_at_addr(unsigned int addr)
{
	if (addr < MAX_ADDR)
	{
		char i;
		char buff = 0;
		SET_OE();
		SET_WE();
		CLR_CE();
	//	digitalWrite(_OE,1);
	//	digitalWrite(_WE,1);
	//	digitalWrite(_CE,0);

		for (i=0; i< 15; i++)
			digitalWrite(eeprom_addr_pins[i],get_bit(addr, i));
		for (i=0; i< 8; i++)
			pinMode(eeprom_data_pins[i], INPUT);
		CLR_OE();	
	//	digitalWrite(_OE, 0);
		NOP;
		delay(9);
		for (i=0; i<8; i++)
			buff |= digitalRead(eeprom_data_pins[i]) <<i;
		NOP;
		NOP;
		NOP;
		SET_OE();
		SET_CE();
		NOP;
		delay(2);
		return buff;
	}
	return 0;
		
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

		SET_OE();
		CLR_CE();
		CLR_WE();	
	//	digitalWrite(_OE, 1);
	//	digitalWrite(_CE, 0);
	//	digitalWrite(_WE, 0);
		NOP;
		NOP;
		NOP;
		
		SET_CE();
		SET_WE();
		//digitalWrite(_CE, 1);
		//digitalWrite(_WE, 1);
		NOP;
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
			NOP;
			digitalWrite(_CE,0);
			digitalWrite(_WE,0);
			NOP;
			NOP;
			digitalWrite(_CE,1);
			digitalWrite(_WE,1);
			NOP;
		}
		unsigned long end_time = millis();
		Serial.print("Time difference (millis): ");
		Serial.println(end_time-init_time);
		digitalWrite(_CE,1);
		digitalWrite(_WE,1);
		NOP;
	}

}
