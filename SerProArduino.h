/*
 SerPro - A serial protocol for arduino intercommunication
 Copyright (C) 2009,2010 Alvaro Lopes <alvieboy@alvie.com>

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

#ifndef __SERPRO_ARDUINO_H__
#define __SERPRO_ARDUINO_H__

#include <inttypes.h>

#ifdef AVR
#include <avr/io.h>
#endif

#define SERPROLIBRARY

#include <serpro/SerPro.h>
#include <serpro/SerProHDLC.h>

#include <WProgram.h>


struct SerialWrapper
{
public:
	static inline void write(uint8_t v) {
		Serial.write(v);
	}
	static inline void write(const uint8_t *buf,int size) {
		Serial.write(buf,size);
	}
	static void flush() {
	}
};

struct HDLCConfig: public HDLC_DefaultConfig
{
	static unsigned int const stationId = 3;
};

#ifndef  SERPRO_ARDUINO_MAXFUNCTIONS
# define SERPRO_ARDUINO_MAXFUNCTIONS 16
#endif

#ifndef  SERPRO_ARDUINO_BUFFERSIZE
# define SERPRO_ARDUINO_BUFFERSIZE 16
#endif


#define SERPRO_ARDUINO_BEGIN(DUMMY)   \
struct SerProConfig {                                                     \
	static unsigned int const maxFunctions = SERPRO_ARDUINO_MAXFUNCTIONS; \
	static unsigned int const maxPacketSize = SERPRO_ARDUINO_BUFFERSIZE;  \
	static SerProImplementationType const implementationType = Slave;     \
	typedef HDLCConfig HDLC; /* HDLC configuration */                     \
}; \
DECLARE_SERPRO(SerProConfig,SerialWrapper,SerProHDLC,SerPro)



#define SERPRO_ARDUINO_END(DUMMY) \
	IMPLEMENT_SERPRO(SERPRO_ARDUINO_MAXFUNCTIONS,SerPro,SerProHDLC)

#endif
