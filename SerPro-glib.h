/*
 SerPro - A serial protocol for arduino intercommunication
 Copyright (C) 2009 Alvaro Lopes <alvieboy@alvie.com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General
 Public License along with this library; if not, write to the
 Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301 USA
 */

#ifndef __SERPRO_GLIB_H__
#define __SERPRO_GLIB_H__

#ifdef INSERPRO
#include "SerPro.h"
#include "SerProHDLC.h"
#else
#include <SerPro/SerPro.h>
#include <SerPro/SerProHDLC.h>
#endif

#include <iostream>
#include <glib.h>
#include <termios.h>
#include <sys/ioctl.h>

class SerialWrapper
{
public:
	static void write(uint8_t v);
	static void write(const unsigned char *buf, unsigned int size);
	static void flush();
	static bool waitEvents(bool block);
};

class GLIBTimer
{
public:
	typedef guint timer_t;
	static timer_t addTimer( int (*cb)(void*), int milisseconds, void *data=0);
	static timer_t cancelTimer(const timer_t &t);
	static inline bool defined(const timer_t &t){
		return t != 0;
	}
};

struct HDLCConfig: public HDLC_DefaultConfig
{
	static unsigned int const stationId = 0xFF; /* Only for HDLC */
};

struct HDLCConfigMaster: public HDLC_DefaultConfig
{
	static unsigned int const stationId = 0xFF; /* Only for HDLC */
    static HDLCImplementationType const implementationType = FULL;
};

struct SerProConfig {
	static unsigned int const maxFunctions = 255;
	static unsigned int const maxPacketSize = 8192;
	static SerProImplementationType const implementationType = Master;
	typedef HDLCConfigMaster HDLC;
};

struct SerProConfigSlave {
	static unsigned int const maxFunctions = 255;
	static unsigned int const maxPacketSize = 1024;
	static SerProImplementationType const implementationType = Slave;
	typedef HDLCConfig HDLC;
};

#define SERPRO_GLIB_BEGIN() \
   DECLARE_SERPRO_WITH_TIMER( SerProConfig, SerialWrapper, SerProHDLC, GLIBTimer, SerPro)

#define SERPRO_GLIB_SLAVE_BEGIN() \
   DECLARE_SERPRO( SerProConfigSlave, SerialWrapper, SerProHDLC, SerPro)

#define SERPRO_GLIB_END() \
	SERPRO_EVENT(LINK_UP) { SerProGLIB::connectEvent(); } \
	IMPLEMENT_SERPRO(255,SerPro,SerProHDLC); \
	void SerProGLIB::start() { SerPro::connect(); } \
    void SerProGLIB::initLayer() { SerPro::initLayer(); } \
	void SerProGLIB::process(unsigned char c) { SerPro::processData(c); }



struct SerProGLIB
{
	static int init(int fd,speed_t baudrate,GMainLoop *loop=NULL);
	static int init(const char *device,speed_t baudrate,GMainLoop *loop=NULL);
	static void run();
	static void runEvents();
	static void start();
	static void waitConnection();
	static gboolean serial_data_ready(GIOChannel *source, GIOCondition condition, gpointer data);

	static void process(unsigned char);
	static void initLayer();

	static void onConnect(void (*func)(void));
	static void connectEvent();

	static void (*connectCB)(void);

	static GIOChannel *channel;
	static gint watcher;
	static int fd;
	static gboolean in_request, delay_request;
	static GMainLoop *loop;
    static bool connected;
};

#endif
