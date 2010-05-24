#include <iostream>
#include "SerProHDLC.h"
#include "SerProPPP.h"
#include "crc16.h"
#include <termios.h>
class SerialWrapper
{
public:
	static void write(uint8_t v);
	static void write(const unsigned char *buf, unsigned int size);
	static void flush();
};

struct SerProConfig {
	static unsigned int const maxFunctions = 4;
	static unsigned int const maxPacketSize = 128;
	static unsigned int const stationId = 0xff; /* Only for HDLC */
	static SerProImplementationType const implementationType = Master;
};

DECLARE_PPP_SERPRO( SerProConfig, SerialWrapper, SerPro);

IMPLEMENT_SERPRO(SerPro);

void SerialWrapper::write(uint8_t v)
{
	putc(v,stdout);
	//fprintf(stderr,"[>%02x]\n",v);
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
