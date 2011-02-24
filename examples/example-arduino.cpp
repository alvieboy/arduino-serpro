#include <SerPro/SerPro-glib.h>

#define OUTPUT 1

SERPRO_GLIB_BEGIN();

IMPORT_FUNCTION(1, pinMode, void (uint8_t,uint8_t) );
IMPORT_FUNCTION(2, digitalWrite, void (uint8_t,uint8_t) );
IMPORT_FUNCTION(3, digitalRead, int16_t (uint8_t) );
IMPORT_FUNCTION(4, analogWrite, void (uint8_t,int16_t) );

SERPRO_GLIB_END();

const uint8_t ledPin = 13;

gboolean myfunction(gpointer data)
{
	uint8_t val = digitalRead(ledPin);
	/* Invert signal */
	digitalWrite(ledPin, !val);
	return TRUE;
}

void connect() // This is called when protocol connects
{  
	pinMode(ledPin, OUTPUT);
	// Start asynchronous function, called each 1/2 second (500 milisseconds)
	g_timeout_add(500, &myfunction, NULL);
}


int main(int argc, char **argv)
{
	if (argc<2)
		return -1;

	if (SerProGLIB::init(argv[1],B115200)<0)
		return -1;

	SerProGLIB::onConnect( &connect );
	SerProGLIB::start();
	SerProGLIB::run();
}
