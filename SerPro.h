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

#ifndef __SERPRO_H__
#define __SERPRO_H__

#include <stdint.h>
#include <string.h> // For strlen


// FIX THIS - We actually want buffer_size_t to be
// computed buffer_size_t

typedef uint8_t buffer_size_t;

#if __GNUC__ > 4 || \
	(__GNUC__ == 4 && (__GNUC_MINOR__ >= 3 ))
# define MAYBESTATIC
#else
# define MAYBESTATIC static
#endif


struct callback {
	void (*deserialize)(unsigned char *,buffer_size_t&,void(*func)(void));
	void (*func)(void);
};

template<unsigned int number>
struct number_of_bytes {
	static unsigned int const bytes = number/256 > 1 ? 2 : 1;
};

template<unsigned int>
struct best_storage_class {
};

template<>
struct best_storage_class<1> {
	typedef uint8_t type;
};

template<>
struct best_storage_class<2> {
	typedef uint16_t type;
};



template<unsigned int MAX_FUNCTIONS, unsigned int MAX_PACKET_SIZE, class Serial>
struct protocolImplementation
{
	protocolImplementation() { st=SIZE; }

	static callback const callbacks[MAX_FUNCTIONS];
	static unsigned int const maxPacketSize = MAX_PACKET_SIZE;
	static unsigned int const maxFunctions = MAX_FUNCTIONS;

	static void callFunction(int index, unsigned char *data)
	{
		buffer_size_t pos = 0;
		callbacks[index].deserialize(data,pos,callbacks[index].func);
	}
	/* Serial processor state */
	enum state {
		SIZE,
		SIZE2,
		COMMAND,
		PAYLOAD,
		CKSUM
	};

	/* Buffer */
	static unsigned char pBuf[MAX_PACKET_SIZE];
	
	typedef uint8_t checksum_t;
	typedef uint8_t command_t;
	typedef typename best_storage_class<number_of_bytes<MAX_PACKET_SIZE>::bytes>::type buffer_size_t;
	typedef uint16_t packet_size_t;

	static buffer_size_t pBufPtr;
	static checksum_t cksum;
	static command_t command;
	static packet_size_t pSize;

	static enum state st;

	static void sendPacket(command_t const command, unsigned char * const buf, packet_size_t const size)
	{
		checksum_t cksum=command;
		packet_size_t i;
		packet_size_t rsize = size;

		// TODO: improve this to be more generic.

		rsize++;
		if (rsize>127) {
			rsize |= 0x8000; // Set MSBit on MSB
			cksum^= (rsize>>8);
			Serial::write((rsize>>8)&0xff);
		}
		cksum^= (rsize&0xff);
		Serial::write(rsize&0xff);

		Serial::write(command);

		rsize=size;

		for (i=0;i<rsize;i++) {
			cksum^=buf[i];
			Serial::write(buf[i]);
		}
		Serial::write(cksum);
	}

	static void processData(uint8_t bIn)
	{
		cksum^=bIn;

		switch(st) {
		case SIZE:
			cksum = bIn;
			if (bIn==0) {
				break; // Reset procedure.
			}
			if (bIn & 0x80) {
				pSize =((packet_size_t)(bIn&0x7F)<<8);
				st = SIZE2;
			} else {
				pSize = bIn;
				if (bIn>MAX_PACKET_SIZE)
					break;
				pBufPtr = 0;
				st = COMMAND;
			}
			break;

		case SIZE2:
			pSize += bIn;
			if (bIn>MAX_PACKET_SIZE)
				break;
			pBufPtr = 0;
			st = COMMAND;
			break;

		case COMMAND:

			command = bIn;
			pSize--;
			if (pSize>0)
				st = PAYLOAD;
			else
				st = CKSUM;
			break;
		case PAYLOAD:

			pBuf[pBufPtr++] = bIn;
			pSize--;
			if (pSize==0) {
				st = CKSUM;
			}
			break;

		case CKSUM:
			if (cksum==0) {
				processPacket();
			}
			st = SIZE;
		}
	}
	static void processPacket()
	{
		buffer_size_t size = 0;
		callFunction(command, pBuf);
	}

	static void send(command_t command) {
		sendPacket(command, NULL, 0);
	}

	template<typename A>
		static void send(command_t command, A value) {
			buffer_size_t length = 0;
			serialize( pBuf, length, value);
			sendPacket(command, pBuf, length);
		};

	template<typename A,typename B>
	static void send(command_t command, A value_a, B value_b) {
		buffer_size_t length = 0;
		serialize( pBuf, length, value_a);
		serialize( pBuf, length, value_b);
		sendPacket(command, pBuf, length);
	}
};

template<typename A>
static A deserialize(unsigned char *b, buffer_size_t &pos) {
	A value = *(A*)&b[pos];
	pos+=sizeof(A);
	return value;
}

template<typename A>
static void serialize(unsigned char *b, buffer_size_t &pos, A value) {
	*(A*)&b[pos] = value;
	pos+=sizeof(A);
}

template<>
MAYBESTATIC char* deserialize(unsigned char *b, buffer_size_t &pos) {
	char *value = (char*)&b[pos];
	pos+=strlen(value);
	return value;
}

template<>
MAYBESTATIC void serialize(unsigned char *b, buffer_size_t &pos,char *string) {
	strcpy( (char*)&b[pos], string);
	pos+=strlen(string);
}

template<unsigned int INDEX, typename B>
struct deserializer {
	static int const index = INDEX;
	typedef B func_type;
};

template<unsigned int INDEX>
struct deserializer<INDEX, void ()> {
	static void handle(unsigned char *, buffer_size_t &pos, void (*func)(void)) {
		func();
	}
};

template<unsigned int INDEX, typename A,typename B>
struct deserializer<INDEX, void (A,B)> {
	static void handle(unsigned char *b, buffer_size_t &pos, void (*func)(A,B)) {
		A val_a=deserialize<A>(b,pos);
		B val_b=deserialize<B>(b,pos);
		func(val_a,val_b);
	}
};

template<unsigned int INDEX, typename A,typename B, typename C>
struct deserializer<INDEX, void (A,B,C)> {
	static void handle(unsigned char *b, buffer_size_t &pos, void (*func)(A, B, C)) {
		A val_a=deserialize<A>(b,pos);
		B val_b=deserialize<B>(b,pos);
		C val_c=deserialize<C>(b,pos);
		func(val_a, val_b, val_c);
	}
};


template<unsigned int>
struct functionHandler {
	static void handle(void){}
	typedef void (type)(void);
};


#define DECLARE_FUNCTION(x) \
template<> \
struct functionHandler<x> { \
	static void handle

#define END_FUNCTION };


typedef void(*serpro_function_type)(void);
typedef void(*serpro_deserializer_type)(unsigned char*,buffer_size_t&,void(*)(void));

#define EXPAND_DELIM ,
#define EXPAND_VALUE(NUMBER,MAX) \
	{ (serpro_deserializer_type)&deserializer<MAX-NUMBER,typeof(functionHandler<MAX-NUMBER>::handle)>::handle,(serpro_function_type)&functionHandler<MAX-NUMBER>::handle }

#include "preprocessor_table.h"

#define DECLARE_SERPRO(num,maxpacksize,serial) \
	typedef protocolImplementation<num,maxpacksize,serial> SerPro;

#define IMPLEMENT_SERPRO(num,maxpacksize,serial, name) \
	template<> \
	callback const protocolImplementation<num,maxpacksize,serial>::callbacks[] = { \
	DO_EXPAND(num) \
	}; \
	template<> enum protocolImplementation<num,maxpacksize,serial>::state protocolImplementation<num,maxpacksize,serial>::st = protocolImplementation<num,maxpacksize,serial>::SIZE; \
	template<> protocolImplementation<num,maxpacksize,serial>::checksum_t protocolImplementation<num,maxpacksize,serial>::cksum = 0; \
	template<> protocolImplementation<num,maxpacksize,serial>::command_t protocolImplementation<num,maxpacksize,serial>::command = 0; \
	template<> protocolImplementation<num,maxpacksize,serial>::buffer_size_t protocolImplementation<num,maxpacksize,serial>::pBufPtr = 0; \
	template<> protocolImplementation<num,maxpacksize,serial>::packet_size_t protocolImplementation<num,maxpacksize,serial>::pSize = 0; \
	template<> unsigned char protocolImplementation<num,maxpacksize,serial>::pBuf[]={0}; \

/*
 { (serpro_deserializer_type)&deserializer<0,typeof(functionHandler<0>::handle)>::handle,(serpro_function_type)&functionHandler<0>::handle }, \
 { (serpro_deserializer_type)&deserializer<1,typeof(functionHandler<1>::handle)>::handle,(serpro_function_type)&functionHandler<1>::handle }, \
 { (serpro_deserializer_type)&deserializer<2,typeof(functionHandler<2>::handle)>::handle,(serpro_function_type)&functionHandler<2>::handle }, \
 { (serpro_deserializer_type)&deserializer<3,typeof(functionHandler<3>::handle)>::handle,(serpro_function_type)&functionHandler<3>::handle } \
 */


#endif
