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
// computed prior. Because we might have either a 8-bit or 16-bit
// Value.

typedef uint8_t buffer_size_t;

// Since GCC 4.3 we cannot have storage class qualifiers on template
// specializations. However, prior versions require them. I know, it's
// quite an ugly name for the macro...

#if __GNUC__ > 4 || \
	(__GNUC__ == 4 && (__GNUC_MINOR__ >= 3 ))
# define MAYBESTATIC
#else
# define MAYBESTATIC static
#endif

// callback structure. One for each function we handle. We define both
// deserializer and function to call.

struct callback {
	void (*deserialize)(const unsigned char *,buffer_size_t&,void(*func)(void));
	void (*func)(void);
};

// These four templates help us to choose a good storage class for
// the receiving buffer size, based on the maximum message size.

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

/* Fixed-size buffer */

template<unsigned int BUFSIZE>
struct FixedBuffer {
	static unsigned int const size = BUFSIZE;
	unsigned char operator[](int i) { return buffer[i]; }
	unsigned char *buffer;
};

template<typename MyProtocol, typename A>
static inline void serialize(A value) {
	MyProtocol::sendData((unsigned char*)&value,sizeof(value));
}

template<typename MyProtocol>
static inline void serialize(uint8_t value) {
	MyProtocol::sendData(value);
}

/* This is pretty much unsafe... */
template<typename MyProtocol>
MAYBESTATIC void serialize(const char *string) {
	MyProtocol::sendData(string,strlen(string));
}

/*
 Our main class definition.
 TODO: document
 */
template<class Config, class Serial,
template <unsigned int,class Ser,class Implementation> class Protocol >
struct protocolImplementation
{
	typedef Protocol<Config::MAX_PACKET_SIZE,Serial,protocolImplementation> MyProtocol;

	static callback const callbacks[Config::MAX_FUNCTIONS];

	/* Forwarded types */
	typedef typename MyProtocol::command_t command_t;
	typedef typename MyProtocol::buffer_size_t buffer_size_t;
	typedef typename MyProtocol::RawBuffer RawBuffer;

	struct VariableBuffer{
		const unsigned char *buffer;
		unsigned int size;
		VariableBuffer(const unsigned char *b,int s): buffer(b),size(s) {}
	};

	static inline void callFunction(int index, const unsigned char *data, buffer_size_t size)
	{
		buffer_size_t pos = 0;
		callbacks[index].deserialize(data,pos,callbacks[index].func);
	}

	static inline void processPacket(uint8_t command,
							  const unsigned char *buf,
							  buffer_size_t size)
	{
		buffer_size_t sz = 0;
		callFunction(command, buf, sz);
	}

	static inline void processData(uint8_t bIn)
	{
		MyProtocol::processData(bIn);
	}

	static inline void send(command_t command) {
		MyProtocol::sendPacket(command, NULL, 0);
	}

	template<typename A>
		static void send(command_t command, A value) {
			buffer_size_t length = 0;
			MyProtocol::startPacket(command, sizeof(A));
			MyProtocol::sendPreamble();
			serialize<MyProtocol,A>(value);
			MyProtocol::sendPostamble();
		};

	template<typename A>
		static void send(command_t command,RawBuffer value) {
			buffer_size_t length = 0;
			MyProtocol::startPacket(command, value.size);
			MyProtocol::sendPreamble();
			MyProtocol::sendData(value.buffer,value.size);
			MyProtocol::sendPostamble();
		};

	template<typename A>
		static void send(command_t command,VariableBuffer value) {
			buffer_size_t length = 0;
			MyProtocol::startPacket(command, value.size);
			MyProtocol::sendPreamble();
			MyProtocol::sendData(value.buffer,value.size);
			MyProtocol::sendPostamble();
		};

	template<typename A,typename B>
	static void send(command_t command, A value_a, B value_b) {
		buffer_size_t length = 0;
		MyProtocol::startPacket(command, sizeof(A)+sizeof(B));
		MyProtocol::sendPreamble();
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C>
	static void send(command_t command, A value_a, B value_b, C value_c) {
		buffer_size_t length = 0;
		MyProtocol::startPacket(command, sizeof(A)+sizeof(B)+sizeof(C));
		MyProtocol::sendPreamble();
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C,typename D>
	static void send(command_t command, A value_a, B value_b, C value_c,D value_d) {
		buffer_size_t length = 0;
		MyProtocol::startPacket(command, sizeof(A)+sizeof(B)+sizeof(C)+sizeof(D));
		MyProtocol::sendPreamble();
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		serialize<MyProtocol>(value_d);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C,typename D,typename E>
	static void send(command_t command, A value_a, B value_b, C value_c,D value_d,E value_e) {
		buffer_size_t length = 0;
		MyProtocol::startPacket(command, sizeof(A)+sizeof(B)+sizeof(C)+sizeof(D)+sizeof(E));
		MyProtocol::sendPreamble();
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		serialize<MyProtocol>(value_d);
		serialize<MyProtocol>(value_e);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C,typename D,typename E,typename F>
	static void send(command_t command, A value_a, B value_b, C value_c,D value_d,E value_e,F value_f) {
		buffer_size_t length = 0;
		MyProtocol::startPacket(command, sizeof(A)+sizeof(B)+sizeof(C)+sizeof(D)+sizeof(E)+sizeof(F));
		MyProtocol::sendPreamble();
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		serialize<MyProtocol>(value_d);
		serialize<MyProtocol>(value_e);
		serialize<MyProtocol>(value_f);
		MyProtocol::sendPostamble();
	}
};


template<typename A>
struct deserialize {
	static A deser(const unsigned char *b, buffer_size_t &pos);
};

/* To avoid possible errors with unknown structures, we define
 simple deserialization for POT, and leave above undefined.
 */

template<>
struct deserialize<uint8_t> {
	static inline uint8_t deser(const unsigned char *b, buffer_size_t &pos) {
		uint8_t value = *(uint8_t*)&b[pos];
		pos+=sizeof(uint8_t);
		return value;
	}
};

template<>
struct deserialize<int8_t> {
	static inline int8_t deser(const unsigned char *b, buffer_size_t &pos) {
		int8_t value = *(int8_t*)&b[pos];
		pos+=sizeof(int8_t);
		return value;
	}
};

template<>
struct deserialize<uint16_t> {
	static inline uint16_t deser(const unsigned char *b, buffer_size_t &pos) {
		uint16_t value = *(uint16_t*)&b[pos];
		pos+=sizeof(uint16_t);
		return value;
	}
};

template<>
struct deserialize<int16_t> {
	static inline int16_t deser(const unsigned char *b, buffer_size_t &pos) {
		int8_t value = *(int8_t*)&b[pos];
		pos+=sizeof(int16_t);
		return value;
	}
};

template<>
struct deserialize<int32_t> {
	static inline int32_t deser(const unsigned char *b, buffer_size_t &pos) {
		int value = *(int*)&b[pos];
		pos+=sizeof(int32_t);
		return value;
	}
};

template<>
struct deserialize<uint32_t> {
	static inline uint32_t deser(const unsigned char *b, buffer_size_t &pos) {
		uint32_t value = *(int*)&b[pos];
		pos+=sizeof(uint32_t);
		return value;
	}
};

template<>
struct deserialize<char*> {
	static char* deser(const unsigned char *b, buffer_size_t &pos) {
		char *value = (char*)&b[pos];
		pos+=strlen(value);
		return value;
	}
};

template<unsigned int BUFSIZE>
struct deserialize < FixedBuffer<BUFSIZE> > {
	static FixedBuffer<BUFSIZE> deser(const unsigned char *b, buffer_size_t &pos) {
		FixedBuffer<BUFSIZE> buf;
		buf.buffer=(unsigned char*)&b[pos];
		pos+=BUFSIZE;
		return buf;
	}
};

template<unsigned int INDEX, typename B>
struct deserializer {
	static int const index = INDEX;
	typedef B func_type;
};

template<unsigned int INDEX>
struct deserializer<INDEX, void ()> {
	static inline void handle(unsigned char *, buffer_size_t &pos, void (*func)(void)) {
		func();
	}
};

template<unsigned int INDEX, typename A>
struct deserializer<INDEX, void (A)> {
	static inline void handle(unsigned char *b, buffer_size_t &pos, void (*func)(A)) {
		A val_a=deserialize<A>::deser(b,pos);
		func(val_a);
	}
};


template<unsigned int INDEX, typename A,typename B>
struct deserializer<INDEX, void (A,B)> {
	static inline void handle(unsigned char *b, buffer_size_t &pos, void (*func)(A,B)) {
		A val_a=deserialize<A>::deser(b,pos);
		B val_b=deserialize<B>::deser(b,pos);
		func(val_a,val_b);
	}
};

template<unsigned int INDEX, typename A,typename B, typename C>
struct deserializer<INDEX, void (A,B,C)> {
	static inline void handle(unsigned char *b, buffer_size_t &pos, void (*func)(A, B, C)) {
		A val_a=deserialize<A>::deser(b,pos);
		B val_b=deserialize<B>::deser(b,pos);
		C val_c=deserialize<C>::deser(b,pos);
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

#define DECLARE_INLINE_FUNCTION(x) \
template<> \
struct functionHandler<x> { \
	static inline void handle

#define END_FUNCTION };


typedef void(*serpro_function_type)(void);
typedef void(*serpro_deserializer_type)(const unsigned char*,buffer_size_t&,void(*)(void));

#define DECLARE_SERPRO(config,serial,proto,name) \
	typedef protocolImplementation<config,serial,proto> name; \
	template<unsigned int INDEX> \
	struct deserializer<INDEX, void (const name::RawBuffer &)> { \
	static void handle(unsigned char *b, buffer_size_t &pos, void (*func)(const name::RawBuffer &)) { \
		func( name::MyProtocol::getRawBuffer() ); \
	} \
	};\


#define EXPAND_DELIM ,
#define EXPAND_VALUE(NUMBER,MAX) \
	{ (serpro_deserializer_type)&deserializer<MAX-NUMBER,typeof(functionHandler<MAX-NUMBER>::handle)>::handle,(serpro_function_type)&functionHandler<MAX-NUMBER>::handle }

#include "preprocessor_table.h"

#define IMPLEMENT_SERPRO(num,name,proto) \
	template<> \
	callback const name::callbacks[] = { \
	DO_EXPAND(num) \
	}; \
	IMPLEMENT_PROTOCOL_##proto(name)

//template<> name::checksum_t name::cksum = 0; \
//	template<> name::buffer_size_t name::pBufPtr = 0; \
//	template<> name::packet_size_t name::pSize = 0; \
//	template<> unsigned char name::pBuf[]={0};

// template<> name::command_t name::command = 0;

#endif
