#include "SerPro.h"

DECLARE_FUNCTION(0)(int a, int b, int c) {
	std::cerr<<"METHOD 0: A(int) "<<a<<",B(int) "<<b<<",C(int) "<<c<<std::endl;
}
END_FUNCTION

DECLARE_FUNCTION(1)(int a, int b) {
	std::cerr<<"METHOD 1: A(int) "<<a<<",B(int) "<<b<<std::endl;
}
END_FUNCTION

DECLARE_FUNCTION(2)(void) {
	std::cerr<<"METHOD 2: Empty function"<<std::endl;
}
END_FUNCTION

DECLARE_FUNCTION(3)(int a, char *b) {
	std::cerr<<"METHOD 3: A(int) "<<a<<",B(string) "<<b<<std::endl;
}
END_FUNCTION


IMPLEMENT_SERPRO(4,myproto);


int main()
{
	int myints[3];
	myints[0] = 45;
	myints[1] = -60;
	myints[2] = 32178632;

	myproto.callFunction(0, (unsigned char*)&myints );
	myproto.callFunction(1, (unsigned char*)&myints );
	myproto.callFunction(2, NULL);

    char buffer[40];
	sprintf(buffer,"%c%c%c%c%s", 1,1,1,0, "Hello");

	myproto.callFunction(3, (unsigned char*)buffer);
}
