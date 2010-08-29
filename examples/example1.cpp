#include <SerPro.h>
#include <SerPro/SerProHDLC.h>
#include <iostream>
#include <glib.h>

static GIOChannel *channel;
static gint watcher;

class SerialWrapper
{
public:
	static void write(uint8_t v) {
		GError *error = NULL;
		gsize written;
		g_io_channel_write_chars(channel,(const gchar*)&v,sizeof(v),&written,&error);
	}

	static void write(const unsigned char *buf, unsigned int size) {
		GError *error = NULL;
		gsize written;
		g_io_channel_write_chars(channel,(const gchar*)buf,size,&written,&error);
	}
	static void flush() {
		GError *error = NULL;
		g_io_channel_flush(channel,&error);
	}
	static bool waitEvents(bool block) {
		return g_main_context_iteration(NULL, TRUE);
	}
};

class GLIBTimer
{
public:
	typedef guint timer_t;
	static timer_t addTimer( int (*cb)(void*), int milisseconds, void *data=0)
	{
		return g_timeout_add(milisseconds,cb,data);
	}
	static timer_t cancelTimer(const timer_t &t) {
		g_source_remove(t);
		return 0;
	}
	static inline bool defined(const timer_t &t) {
		return t != 0;
	}
};

struct HDLCConfig: public HDLC_DefaultConfig
{
	static unsigned int const stationId = 0xFF; /* Only for HDLC */
};

struct SerProConfig {
	static unsigned int const maxFunctions = 128;
	static unsigned int const maxPacketSize = 1024;
	static SerProImplementationType const implementationType = Master;
	typedef HDLCConfig HDLC;
};

DECLARE_SERPRO_WITH_TIMER( SerProConfig, SerialWrapper, SerProHDLC, GLIBTimer, SerPro);

void function1(int a)
{
	std::cerr<<"Called "<<__FUNCTION__<<" with "<<a<<std::endl;
}
EXPORT_FUNCTION(1, function1);

IMPORT_FUNCTION(2, applySettings, int (int) );


IMPLEMENT_SERPRO(128,SerPro,SerProHDLC);

int main()
{
	int c = applySettings(2);
	SerPro::callFunction(1,(const unsigned char*)&c,sizeof(c));
}
