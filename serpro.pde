#include <SerProArduino.h>

SERPRO_ARDUINO_BEGIN();


void setup()
{
}

void loop()
{
	if (Serial.available()>0) {
		SerPro::processData(Serial.read());
	}
}

SERPRO_ARDUINO_END();
