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

#ifndef __SERPROCOMMON__
#define __SERPROCOMMON__

#include <inttypes.h>

#if defined(AVR) || defined (ZPU)
#define SERPRO_EMBEDDED
#endif

#ifndef SERPRO_EMBEDDED
#include <string.h>
#endif

typedef enum {
	Master,
	Slave
} SerProImplementationType;

class NoTimer
{
public:
	typedef int timer_t;
	static inline timer_t addTimer( int (*cb)(void*), int milisseconds, void *data=0)
	{
		return 0;
	}
	static inline timer_t cancelTimer(const timer_t &t)
	{
		return 0;
	}
	static inline bool defined(const timer_t &t) {
		return false;
	}
};

/* Byte order stuff */

#if defined(ZPU)
# define cpu_to_be32(x) x
# define cpu_to_be16(x) x
# define be16_to_cpu(x) x
# define be32_to_cpu(x) x
#elif defined(AVR)

extern uint16_t bswap_16(uint16_t);
extern uint32_t bswap_32(uint32_t);

# define cpu_to_be32(x) bswap_32(x)
# define cpu_to_be16(x) bswap_16(x)
# define be16_to_cpu(x) bswap_16(x)
# define be32_to_cpu(x) bswap_32(x)

#else
# include <byteswap.h>
# define cpu_to_be32(x) bswap_32(x)
# define cpu_to_be16(x) bswap_16(x)
# define be16_to_cpu(x) bswap_16(x)
# define be32_to_cpu(x) bswap_32(x)
#endif

#ifdef SERPRO_EMBEDDED
struct VariableBuffer{
	const unsigned char *buffer;
	uint32_t size;
	VariableBuffer(const unsigned char *b,uint32_t s): buffer(b),size(s) {
	}
	VariableBuffer(): buffer(0),size(0) {}
	/*VariableBuffer &operator=(const VariableBuffer&s) {
	 } */
};
#else

struct VariableBuffer{
	unsigned char *buffer;
	uint32_t size;
	VariableBuffer(const unsigned char *b,uint32_t s):size(s) {
		if(b) {
			buffer=new unsigned char[s];
			memcpy(buffer,b,s);
		}
	}
	VariableBuffer(): buffer(0),size(0) {}
	/*VariableBuffer &operator=(const VariableBuffer&s) {
	 } */
	~VariableBuffer() {
		if(buffer)
			delete[]buffer;
	}
	VariableBuffer(const VariableBuffer &other) {
		if (buffer)
			delete[] buffer;
		buffer=0;
		size=other.size;

		if (other.buffer) {
			buffer=new unsigned char[size];
			memcpy(buffer,other.buffer,size);
		}
	}
	VariableBuffer &operator=(const VariableBuffer&other) {
		if (buffer)
			delete[] buffer;
		buffer=0;
		size=other.size;

		if (other.buffer) {
			buffer=new unsigned char[size];
			memcpy(buffer,other.buffer,size);
		}
		return *this;
	}
};

#endif



/* Fixed-size buffer */
template<unsigned int BUFSIZE>
struct FixedBuffer {
	static unsigned int const size = BUFSIZE;
	unsigned char operator[](int i) const { return buffer[i]; }
	unsigned char *buffer;
};


#endif
