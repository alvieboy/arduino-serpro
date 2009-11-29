#include <iostream>
#include "SerProHDLC.h"
#include "SerProPacket.h"
#include "SerPro.h"

class SerialWrapper
{
public:
	static void write(uint8_t v);
	static void write(const unsigned char *buf, unsigned int size);
};

struct SerProConfig {
	static unsigned int const MAX_FUNCTIONS = 4;
	static unsigned int const MAX_PACKET_SIZE = 32;
};

DECLARE_SERPRO( SerProConfig, SerialWrapper, SerProHDLC, SerPro);

DECLARE_FUNCTION(0)(int a, int b, int c) {
	std::cerr<<"METHOD 0: A(int) "<<a<<",B(int) "<<b<<",C(int) "<<c<<std::endl;
	SerPro::send(1,a,c,b);
}
END_FUNCTION

DECLARE_FUNCTION(1)( int a, int b, int c) {
	std::cerr<<"METHOD 1: A(int) "<<a<<",B(int) "<<b<<",C(int) "<<c<<std::endl;
	SerPro::send(2,c,b,a);
}
END_FUNCTION

DECLARE_FUNCTION(2)( int a, int b, int c ) {
	std::cerr<<"METHOD 2: Empty function"<<std::endl;
	SerPro::send(3);
}
END_FUNCTION

DECLARE_FUNCTION(3)(void) {
	std::cerr<<"METHOD 3: END"<<std::endl;
}
END_FUNCTION


IMPLEMENT_SERPRO(4,SerPro,SerProHDLC);

static SerPro serpro;

void SerialWrapper::write(uint8_t v)
{
	SerPro::processData(v);
}

void SerialWrapper::write(const unsigned char *buf, unsigned int size)
{
}

int main()
{
	int myints[3];
	myints[0] = 0x7E7D7D7E; // Special case. Will cause lots of escapes
	myints[1] = -60;
	myints[2] = 32178632;

	char buffer[40];
	serpro.callFunction(0, (unsigned char*)&myints,1 );
}
