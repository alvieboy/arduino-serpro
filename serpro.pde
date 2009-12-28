#include "SerProHDLC.h"
#include "SerProPPP.h"
#include "crc16.h"

const int ledPin = 13;

class SerialWrapper
{
public:
	static void write(uint8_t v) { Serial.write(v); }
	static void write(const unsigned char *buf, unsigned int size) { Serial.write(buf,size); }
	static void flush() {}
};

struct SerProConfig {
	static unsigned int const maxFunctions = 4;
	static unsigned int const maxPacketSize = 128;
	static unsigned int const stationId = 0xff; /* Only for HDLC */
};

DECLARE_PPP_SERPRO( SerProConfig, SerialWrapper, SerPro);

IMPLEMENT_SERPRO(SerPro);

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
