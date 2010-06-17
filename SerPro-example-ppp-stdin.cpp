#include <iostream>
#include "IPStack.h"
#include <termios.h>
#include "Socket.h"

class SerialWrapper
{
public:
	static void write(uint8_t v);
	static void write(const unsigned char *buf, unsigned int size);
	static void flush();
};

DECLARE_IP_STACK(SerialWrapper);

IMPLEMENT_IP_STACK()

void SerialWrapper::write(uint8_t v)
{
	putc(v,stdout);
}

void SerialWrapper::flush()
{
	fflush(stdout);
	tcdrain(1);
}

void SerialWrapper::write(const unsigned char *buf, unsigned int size)
{
	for (int i=0;i<size;i++)
		write(buf[i]);
}


int main()
{
	int r;
	struct termios termset;
	tcgetattr(1, &termset);

	termset.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
						 | INLCR | IGNCR | ICRNL | IXON);
	termset.c_oflag &= ~OPOST;
	termset.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termset.c_cflag &= ~(CSIZE | PARENB);
	termset.c_cflag |= CS8;

	tcsetattr(1,TCSANOW,&termset);

	setvbuf(stdin,0,_IONBF,0);
	setvbuf(stdout,0,_IONBF,0);
	while((r = getc(stdin))>0) {
		//fprintf(stderr,"[<%02x]\n",r&0xff);
		SerPro::processData(r&0xff);
	}
}
