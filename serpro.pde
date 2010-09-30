#include <SerProArduino.h>

SERPRO_ARDUINO_BEGIN();


SERPRO_ARDUINO_END();

void setup()
{
}

void loop()
{
	if (Serial.available()>0) {
		SerPro::processData(Serial.read());
	}
}
