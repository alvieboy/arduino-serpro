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

#include <sys/types.h>
#include <iostream>

struct callback {
	void (*deserialize)(unsigned char *,size_t&,void(*func)(void));
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
	typedef u_int8_t type;
};

template<>
struct best_storage_class<2> {
	typedef u_int16_t type;
};


template<unsigned int MAX_FUNCTIONS, unsigned int MAX_PACKET_SIZE>
struct protocolImplementation
{
	static callback const callbacks[MAX_FUNCTIONS];
	static unsigned int const maxPacketSize = MAX_PACKET_SIZE;
	static unsigned int const maxFunctions = MAX_FUNCTIONS;

	static void callFunction(int index, unsigned char *data)
	{
		size_t pos = 0;
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
	/* Checksum */
	typedef u_int8_t checksum_t;
	static checksum_t cksum;

	static typename best_storage_class<number_of_bytes<MAX_PACKET_SIZE>::bytes>::type pBufPtr, pSize;
	static u_int8_t command;
	static enum state st;
};


template<unsigned int INDEX, typename B>
struct deserializer {
	static int const index = INDEX;
	typedef B func_type;
};

template<unsigned int INDEX>
struct deserializer<INDEX, void ()> {
	static void handle(unsigned char *, size_t &pos, void (*func)(void)) {
		func();
	}
};

template<typename A>
static A deserialize(unsigned char *b, size_t &pos) {
	A value = *(A*)&b[pos];
	pos+=sizeof(A);
	return value;
}

template<>
static char* deserialize(unsigned char *b, size_t &pos) {
	char *value = (char*)&b[pos];
	pos+=strlen(value);
	return value;
}

template<unsigned int INDEX, typename A,typename B>
struct deserializer<INDEX, void (A,B)> {
	static void handle(unsigned char *b, size_t &pos, void (*func)(A,B)) {
		A val_a=deserialize<A>(b,pos);
		B val_b=deserialize<B>(b,pos);
		func(val_a,val_b);
	}
};

template<unsigned int INDEX, typename A,typename B, typename C>
struct deserializer<INDEX, void (A,B,C)> {
	static void handle(unsigned char *b, size_t &pos, void (*func)(A, B, C)) {
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
typedef void(*serpro_deserializer_type)(unsigned char*,size_t&,void(*)(void));

#define EXPAND_DELIM ,
#define EXPAND_VALUE(NUMBER,MAX) \
	{ (serpro_deserializer_type)&deserializer<MAX-NUMBER,typeof(functionHandler<MAX-NUMBER>::handle)>::handle,(serpro_function_type)&functionHandler<MAX-NUMBER>::handle }

#include "preprocessor_table.h"

#define IMPLEMENT_SERPRO(num,maxpacksize,name) \
	static protocolImplementation<num,maxpacksize> name; \
	template<> \
	callback const protocolImplementation<num,maxpacksize>::callbacks[] = { \
	DO_EXPAND(num) \
	};


/*
 { (serpro_deserializer_type)&deserializer<0,typeof(functionHandler<0>::handle)>::handle,(serpro_function_type)&functionHandler<0>::handle }, \
 { (serpro_deserializer_type)&deserializer<1,typeof(functionHandler<1>::handle)>::handle,(serpro_function_type)&functionHandler<1>::handle }, \
 { (serpro_deserializer_type)&deserializer<2,typeof(functionHandler<2>::handle)>::handle,(serpro_function_type)&functionHandler<2>::handle }, \
 { (serpro_deserializer_type)&deserializer<3,typeof(functionHandler<3>::handle)>::handle,(serpro_function_type)&functionHandler<3>::handle } \
 */


#endif
