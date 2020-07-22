#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Linux Headers
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

/*
 *
 * This is the other half for the eeprom programmer using an arduino. It will be bitstreaming a given binary file to the arduino and the arduino will be going through the process of actually writing the data the eeprom. Not that there will be several handshakes that will be occuring as the arduino has limited storage capability 
 * Note this writer will only be usable under linux as I do not wish to experiment with Window's serial libraries
 *
*/
// execution parameters: ./binary_eeprom_writer [binary file] [serial device]

//declarations of functions:
int bin_size_counter(char *);
void stream_bin_file(char *);
int open_serial(int *, char *, struct termios *);
int issue_test_comm(int *);
int write_binary_file(int *, FILE *, int);
int issue_test_write_read(int *);

union short_parser{
	unsigned short data;
	unsigned char data_char[2];
};

//global variables (not good programming practice but not horrible in terms of embedded programming

int main (int argc, char **argv)
{
	int serial_port;
	struct termios tty;
	if (argc == 3)
	{
	FILE *fp; 
	if ((fp = fopen(argv[1],"rb")) == NULL)
	{
		printf("Unable to open the specified binary file: %s \n", argv[1]);
		return 1;
	}
		printf("file size: %d\n",bin_size_counter(argv[1]));
		if (open_serial(&serial_port, argv[2], &tty) == 0) {
			//issue_test_comm(&serial_port);
			write_binary_file(&serial_port, fp, bin_size_counter(argv[1]));
			//write_binary_file(&serial_port, fp, bin_size_counter(argv[1]));
			close(serial_port);
		}

	}
	else
	{
		printf("incorrect number of arguments given\n");
		return 1;
	}
	return 0;
}

int open_serial(int *serial_port, char * serial_path, struct termios * tty)
{
	*serial_port = open(serial_path, O_RDWR);
	if (serial_port <0)
	{
		printf("Error %i, from opening path - %s, string error: %s\n", errno, serial_path, strerror(errno));	
		return errno;
	}
	
	memset(tty, 0, sizeof *tty);
	
	if (tcgetattr(*serial_port, tty) != 0)
	{
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		return errno;
	}
	//settings the various config settings
	tty->c_cflag &= ~PARENB; //clear parity bit (disable it )
	tty->c_cflag &= ~CSTOPB; //set to only one stop bit
	tty->c_cflag |= CS8; //set 8 bits per data packet
	tty->c_cflag &= ~CRTSCTS; //disable flow control
	tty->c_cflag |= CREAD | CLOCAL; // allows for reading data and disables carrier detect
	tty->c_lflag = ~ICANON; //disable some canonical linux stuff
	tty->c_lflag &= ~ECHO; // disable echo
	tty->c_lflag &= ~ECHOE; // disable erasure
	tty->c_lflag &= ~ECHONL; // disable new-line echo
	tty->c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty->c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	tty->c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty->c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	tty->c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty->c_cc[VMIN] = 0;
	cfsetispeed(tty, B9600); //set the input baudrate cfsetospeed(tty, B9600); //set output baudrate if (tcsetattr(*serial_port, TCSANOW, tty) != 0) {
	if (tcsetattr(*serial_port, TCSANOW, tty) != 0) {
		printf("Error %i from tcsetattr: %s\n", errno,strerror(errno));
		return errno;
	}
	sleep(1);

	return 0;
}

int issue_test_comm(int *serial_port)
{
	char buffer;
	write(*serial_port, "t", 1);
	usleep(1000);
	int num_bytes = read(*serial_port, &buffer, 1);

	if (num_bytes <0)
	{
		printf("Error reading: %s\n", strerror(errno));
		return errno;
	}
		
	printf("receive %i bytes. Received Message: %c", num_bytes, buffer);	
	return 0;
}

int issue_test_write_read(int *serial_port)
{
	char buffer;
	write(*serial_port, "w", 1);
	sleep(1);
	write(*serial_port, "r", 1);
	return 0;
}


int write_binary_file(int *serial_port, FILE *fp, int size_bits)
{
	union short_parser parser;
	union short_parser parser2;
	parser.data  = (unsigned short) (size_bits%8 != 0 ? (((double) size_bits)/8.00)+1 : size_bits/8);
	char *buffer_file =  (char*) malloc(sizeof(char)*parser.data);
	int bytes_read = fread(buffer_file, sizeof(char), parser.data, fp);

	printf("bytes_read size: %i\n", parser.data);
	printf("1-%#x, 2-%#x \n", (int)parser.data_char[0], (int)parser.data_char[1]);
	write(*serial_port, "b", 1);	
	write(*serial_port, parser.data_char,2); 	
	int num_bytes = read(*serial_port, &parser2.data_char, 2);
	printf("received sized: %i\n", parser2.data);
	printf("1: %#x, 2: %#x \n", (int)parser2.data_char[0], (int)parser2.data_char[1]);
	if (parser2.data == parser.data)
		write(*serial_port, "y", 1);
	char query_buff;
	int i =0;

	if (parser.data/32 !=0)	
	{
	while (i< (parser.data/32)*32)
	{
		read(*serial_port, &query_buff,1);
		if (query_buff == 'r')
		{
			int j;
			char chunk_buff[32];
			for (j=0; j<32;j++)
				chunk_buff[j] = buffer_file[i+j];
			write(*serial_port, chunk_buff, 32);	
		}
	}
	}
	if (parser.data-i-1 > 0)
	{
		read(*serial_port, &query_buff, 1);
		if (query_buff == 'r')
		{
			int j;
			char chunk_buff[parser.data-i-1];
			for (j=0; j<parser.data-i-1; j++)
				chunk_buff[j] = buffer_file[i+j];
			write(*serial_port, chunk_buff, parser.data-i-1);
		}
	}				
	
	free(buffer_file);	
	return 0;
}


//returns the number of bits in the file note, that there is always 1 extra bit in a non-empty binary file
int bin_size_counter(char *filename)
{
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}
