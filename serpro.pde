#include "SerPro.h"

class SerialWrapper
{
public:
	static inline void write(uint8_t v){
		Serial.write(v);
	};
};

DECLARE_SERPRO(4,32,SerialWrapper);

DECLARE_FUNCTION(0)(int a, int b, int c) {
	SerPro::send(0, a+b+c);
}
END_FUNCTION

DECLARE_FUNCTION(1)(int a, int b) {
	SerPro::send(1, a+b);
}
END_FUNCTION

DECLARE_FUNCTION(2)(void) {
	SerPro::send(2);
}
END_FUNCTION

DECLARE_FUNCTION(3)(int a, char *b) {
	SerPro::send(3, b, a+1);
}
END_FUNCTION


IMPLEMENT_SERPRO(4,32,SerialWrapper,myproto);


void setup()
{
}

void loop()
{
}
