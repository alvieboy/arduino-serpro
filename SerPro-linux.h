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

#ifndef __SERPRO_LINUX_H__
#define __SERPRO_LINUX_H__

#ifdef INSERPRO
#include "SerPro.h"
#include "SerProHDLC.h"
#else
#include <SerPro/SerPro.h>
#include <SerPro/SerProHDLC.h>
#endif

#include <iostream>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <set>

class SerialWrapper
{
public:
	static void write(uint8_t v);
	static void write(const unsigned char *buf, unsigned int size);
	static void flush();
	static bool waitEvents(bool block);
};

class LinuxTimer
{
public:
	typedef int timer_t;
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
	static unsigned int const maxPacketSize = 1024;
	static SerProImplementationType const implementationType = Master;
	typedef HDLCConfigMaster HDLC;
};

struct SerProConfigSlave {
	static unsigned int const maxFunctions = 255;
	static unsigned int const maxPacketSize = 1024;
	static SerProImplementationType const implementationType = Slave;
	typedef HDLCConfig HDLC;
};

#define SERPRO_LINUX_BEGIN() \
   DECLARE_SERPRO_WITH_TIMER( SerProConfig, SerialWrapper, SerProHDLC, LinuxTimer, SerPro)

#define SERPRO_LINUX_SLAVE_BEGIN() \
   DECLARE_SERPRO( SerProConfigSlave, SerialWrapper, SerProHDLC, SerPro)

#define SERPRO_LINUX_END() \
	IMPLEMENT_SERPRO(255,SerPro,SerProHDLC); \
	void SerProLinux::start() { SerPro::connect(); } \
	void SerProLinux::process(unsigned char c) { SerPro::processData(c); } \
	SERPRO_EVENT(LINK_UP) { SerProLinux::connectEvent(); }


struct TimerEntry
{
	struct timeval expires;
	int id;
	bool (*callback)(void*);
	void *data;
	bool operator()(const TimerEntry &a,const TimerEntry &b) {
		return timercmp(&a.expires,&b.expires,<);
	}
	bool expired(const struct timeval &now) const {
		return timercmp(&expires,&now,<);
	}
	bool expires_in(const struct timeval &now, struct timeval &will) const {
		timersub(&expires,&now,&will);
	}
};


struct SerProLinux
{
	static int init(int fd,speed_t baudrate);
	static int init(const char *device,speed_t baudrate);
	static void run();
	static void runEvents();
	static void start();
	static void waitConnection();
	static bool serial_data_ready();

	static void process(unsigned char);

	static void onConnect(void (*func)(void));
	static void connectEvent();

	static void (*connectCB)(void);
	static void loop();

	static int fd;
	static bool in_request, delay_request;
	static bool connected;
	static int ids;
	static std::set<TimerEntry,TimerEntry> events;
};

#endif
