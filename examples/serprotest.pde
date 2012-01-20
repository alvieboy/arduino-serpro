#include <SerPro3.h>

SERPRO_ARDUINO_BEGIN();

EXPORT_FUNCTION(1, pinMode);
EXPORT_FUNCTION(2, digitalWrite);
EXPORT_FUNCTION(3, digitalRead);
EXPORT_FUNCTION(4, analogWrite);

SERPRO_ARDUINO_END();

void setup()
{
	Serial.begin(115200);
}

void loop()
{
	if (Serial.available()>0) {
		SerPro::processData(Serial.read());
	}
}
