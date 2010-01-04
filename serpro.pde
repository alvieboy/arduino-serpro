#include "SerPro.h"
#include "SerProPacket.h"
#include "SerProHDLC.h"

class SerialWrapper
{
public:
	static inline void write(uint8_t v){
		Serial.write(v);
	};
	static inline void flush(){
	};

};

// 4 functions
// 32 bytes maximum receive buffer size

struct SerProConfig {
	static unsigned int const maxFunctions = 4;
	static unsigned int const maxPacketSize = 32;
	static unsigned int const stationId = 3;
};

// SerialWrapper is class to handle tx
// Name of class is SerPro
// Protocol type is SerProPacket or SerProHDLC

DECLARE_SERPRO(SerProConfig,SerialWrapper,SerProHDLC,SerPro);

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
	SerPro::send<uint8_t>(4, a+1);  // Enforce int argument to be a uint8_t
}
END_FUNCTION


// Implementation - 4 functions, name SerPro, protocol SerProHDLC.
// Make sure this agrees with DECLARE statement.

IMPLEMENT_SERPRO(4,SerPro,SerProHDLC);

const int ledPin = 13;

void setup()
{
	Serial.begin(115200);
	pinMode(ledPin,OUTPUT);
}



void loop()
{
	if (Serial.available()>0) {
		SerPro::processData(Serial.read());
	}
}
