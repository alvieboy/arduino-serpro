#include "SerPro.h"
#include <iostream>

class SerialWrapper
{
public:
	static inline void write(uint8_t v){
		std::cerr<<"W: "<<(int)v<<std::endl;
	};
};

DECLARE_SERPRO(4,32,SerialWrapper);

DECLARE_FUNCTION(0)(int a, int b, int c) {
	std::cerr<<"METHOD 0: A(int) "<<a<<",B(int) "<<b<<",C(int) "<<c<<std::endl;
	SerPro::send<uint8_t>(0,a+c+b);
}
END_FUNCTION

DECLARE_FUNCTION(1)(int a, int b) {
	std::cerr<<"METHOD 1: A(int) "<<a<<",B(int) "<<b<<std::endl;
}
END_FUNCTION

DECLARE_FUNCTION(2)(void) {
	std::cerr<<"METHOD 2: Empty function"<<std::endl;
	SerPro::send(2);
}
END_FUNCTION

DECLARE_FUNCTION(3)(int a, char *b) {
	std::cerr<<"METHOD 3: A(int) "<<a<<",B(string) "<<b<<std::endl;
	SerPro::send<char*,uint8_t>(3, b, a+1);
}
END_FUNCTION


IMPLEMENT_SERPRO(4,32,SerialWrapper,myproto);


int main()
{
	int myints[3];
	myints[0] = 45;
	myints[1] = -60;
	myints[2] = 32178632;

	SerPro::callFunction(0, (unsigned char*)&myints );
	SerPro::callFunction(1, (unsigned char*)&myints );
	SerPro::callFunction(2, NULL);

    char buffer[40];
	sprintf(buffer,"%c%c%c%c%s", 1,1,1,0, "Hello");

	SerPro::callFunction(3, (unsigned char*)buffer);

	SerPro::processData(0);
}
