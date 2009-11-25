#include <iostream>
#include "SerProHDLC.h"
#include "SerProPacket.h"
#include "SerPro.h"

class SerialWrapper
{
public:
	static inline void write(uint8_t v){
		std::cerr<<"W: "<<(int)v<<std::endl;
	};
};

struct SerProConfig {
	static unsigned int const MAX_FUNCTIONS = 4;
	static unsigned int const MAX_PACKET_SIZE = 32;
};

DECLARE_SERPRO( SerProConfig, SerialWrapper, SerProPacket, SerPro);

DECLARE_FUNCTION(0)(int a, int b, int c) {
	std::cerr<<"METHOD 0: A(int) "<<a<<",B(int) "<<b<<",C(int) "<<c<<std::endl;
	SerPro::send<uint8_t>(0,a+c+b);
}
END_FUNCTION

DECLARE_FUNCTION(1)( FixedBuffer<4> a,char *b) {
	std::cerr<<"METHOD 0: FixedBuffer "<<(int)a[0]
		<<" "<<(int)a[1]<<" "<<(int)a[2]<<" "<<(int)a[3];
	std::cerr<<", String "<<b<<std::endl;
}
END_FUNCTION

DECLARE_FUNCTION(2)( void ) {
	std::cerr<<"METHOD 2: Empty function"<<std::endl;
	SerPro::send(2);
}
END_FUNCTION

DECLARE_FUNCTION(3)(int a, char *b) {
	std::cerr<<"METHOD 3: A(int) "<<a<<",B(string) "<<b<<std::endl;
	SerPro::send<char*,uint8_t>(3, b, a+1);
}
END_FUNCTION


IMPLEMENT_SERPRO(4,SerPro,SerProPacket);

static SerPro serpro;

int main()
{
	int myints[3];
	myints[0] = 45;
	myints[1] = -60;
	myints[2] = 32178632;

	char buffer[40];
	serpro.callFunction(0, (unsigned char*)&myints,1 );
	serpro.callFunction(2, (unsigned char*)buffer,1);


	sprintf(buffer,"%c%c%c%c%s", 1,1,1,0, "Hello");

	serpro.callFunction(3, (unsigned char*)buffer,1);
	serpro.callFunction(1, (unsigned char*)buffer,1);

	serpro.processData(0);
}
