#include <SerPro3.h>

SERPRO_ARDUINO_BEGIN();
           
#define LEDPIN 11

void setup()
{
    pinMode(LEDPIN,OUTPUT);
}

void loop()
{
	if (Serial.available()>0) {
		SerPro::processData(Serial.read());
	}
}

void setLed(uint8_t value)
{
	digitalWrite(LEDPIN,value);
}

EXPORT_FUNCTION(1,setLed); /* Export this function in slot 1 */


SERPRO_ARDUINO_END();
