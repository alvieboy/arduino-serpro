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
#include "Packet.h"
#include "SerProCommon.h"

// Since GCC 4.3 we cannot have storage class qualifiers on template
// specializations. However, prior versions require them. I know, it's
// quite an ugly name for the macro...

#if __GNUC__ > 4 || \
	(__GNUC__ == 4 && (__GNUC_MINOR__ >= 3 ))
# define MAYBESTATIC
#else
# define MAYBESTATIC static
#endif

#ifdef AVR
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif

/* Fixed-size buffer */

template<unsigned int BUFSIZE>
struct FixedBuffer {
	static unsigned int const size = BUFSIZE;
	unsigned char operator[](int i) { return buffer[i]; }
	unsigned char *buffer;
};

struct VariableBuffer{
	const unsigned char *buffer;
	unsigned int size;
	VariableBuffer(const unsigned char *b,int s): buffer(b),size(s) {
	}
};

template<typename MyProtocol, class A>
static inline void serialize(const A *const value) {
	MyProtocol::sendData((unsigned char*)value,sizeof(A));
}

template<typename MyProtocol>
static inline void serialize(uint8_t value) {
	MyProtocol::sendData(value);
}

template<typename MyProtocol>
static inline void serialize(const uint16_t &value) {
	MyProtocol::sendData((unsigned char*)&value,sizeof(uint16_t));
}

template<typename MyProtocol>
static inline void serialize(const uint32_t &value) {
	MyProtocol::sendData((unsigned char*)&value,sizeof(uint32_t));
}

template<typename MyProtocol>
static inline void serialize(int8_t value) {
	MyProtocol::sendData((uint8_t)value);
}

template<typename MyProtocol>
static inline void serialize(const int16_t &value) {
	MyProtocol::sendData((unsigned char*)&value,sizeof(int16_t));
}

template<typename MyProtocol>
static inline void serialize(const int32_t &value) {
	MyProtocol::sendData((unsigned char*)&value,sizeof(int32_t));
}

template<typename MyProtocol>
static inline void serialize(const VariableBuffer &buf) {
	MyProtocol::sendData(buf.buffer, buf.size);
}

/* This is pretty much unsafe... */
#if 0
template<typename MyProtocol>
MAYBESTATIC void serialize(const char *string) {
	MyProtocol::sendData(string,strlen(string));
}
#endif

/*
 Our main class definition.
 TODO: document
 */
template<class Config, class Serial,
template <class Config, class Ser,class Implementation, class Timer> class Protocol,class Timer=NoTimer >
struct protocolImplementation
{
	typedef Protocol<Config,Serial,protocolImplementation,Timer> MyProtocol;
	/* Forwarded types */
	typedef typename MyProtocol::command_t command_t;
	typedef typename MyProtocol::buffer_size_t buffer_size_t;
	typedef typename MyProtocol::RawBuffer RawBuffer;

	// callback structure. One for each function we handle. 

	typedef void (*func_type)(const unsigned char *, buffer_size_t&);

	struct callback {
		func_type func;
	};

	static callback const PROGMEM callbacks[Config::maxFunctions];

	struct VariableBuffer{
		const unsigned char *buffer;
		unsigned int size;
		VariableBuffer(const unsigned char *b,int s): buffer(b),size(s) {
		}
	};

	static void connect() {
		MyProtocol::startLink();
	}

	static inline void deferReply()
	{
		MyProtocol::deferReply();
	}

	static inline void callFunction(command_t index, const unsigned char *data, buffer_size_t size)
	{
		buffer_size_t pos = 0;
#ifdef AVR
		func_type func = (func_type)pgm_read_word(&callbacks[index].func);
		func(data,pos);
#else
		callbacks[index].func(data,pos);
#endif
	}

	/* Only for Master */
	static void sendPacket(command_t command, uint8_t *payload, size_t payload_size) {

		Packet *p = MyProtocol::createPacket();
		p->append(command);
		p->appendBuffer(payload,payload_size);

		MyProtocol::queueTransmit(p);
	}

	static void sendPacket(command_t command) {
		Packet *p = MyProtocol::createPacket();
		p->append(command);
		MyProtocol::queueTransmit(p);
	}

	template <class A>
	static void sendPacket(command_t command, const A&value) {
		Packet *p = MyProtocol::createPacket();
		p->append(command);
		p->append(value);
		MyProtocol::queueTransmit(p);
	}

	template <class A,class B>
	static void sendPacket(command_t command, const A &value_a, const B &value_b) {
		Packet *p = MyProtocol::createPacket();
		p->append(command);
		p->append(value_a,value_b);
		MyProtocol::queueTransmit(p);
	}

	template <class A,class B,class C>
	static void sendPacket(command_t command, const A &value_a, const B &value_b,
						   const C&value_c) {
		Packet *p = MyProtocol::createPacket();
		p->append(command);
		p->append(value_a,value_b,value_c);
		MyProtocol::queueTransmit(p);
	}

	template <class A,class B,class C,class D>
	static void sendPacket(command_t command, const A &value_a, const B &value_b,
						   const C&value_c, const D &value_d) {
		Packet *p = MyProtocol::createPacket();
		p->append(command);
		p->append(value_a,value_b,value_c,value_d);
		MyProtocol::queueTransmit(p);
	}

	template <class A,class B,class C,class D,class E>
	static void sendPacket(command_t command, const A &value_a, const B &value_b,
						   const C&value_c, const D &value_d, const E &value_e) {
		Packet *p = MyProtocol::createPacket();
		p->append(command);
		p->append(value_a,value_b,value_c,value_d);
		MyProtocol::queueTransmit(p);
	}


	static inline void processPacket(const unsigned char *buf,
									 buffer_size_t size)
	{
		buffer_size_t sz = 0; // TODO - put packet size here.

		callFunction(buf[0], buf+1, sz-1);
	}

	static inline void processData(uint8_t bIn)
	{
		MyProtocol::processData(bIn);
	}

	static inline void send(command_t command) {
		MyProtocol::startPacket(sizeof(command));
		MyProtocol::sendPreamble();
		MyProtocol::sendData(command);
		MyProtocol::sendPostamble();
	}

	template<typename A>
		static void send(command_t command, A value) {
			MyProtocol::startPacket(sizeof(A)+sizeof(command));
			MyProtocol::sendPreamble();
			MyProtocol::sendData(command);
			serialize<MyProtocol>(value);
			MyProtocol::sendPostamble();
		};

	template<typename A,typename B>
	static void send(command_t command, const A value_a, const B value_b) {
		MyProtocol::startPacket(sizeof(command)+sizeof(A)+sizeof(B));
		MyProtocol::sendPreamble();
		MyProtocol::sendData(command);
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C>
	static void send(command_t command, const A value_a, const B value_b, const C value_c) {
		MyProtocol::startPacket(sizeof(command)+ sizeof(A)+sizeof(B)+sizeof(C));
		MyProtocol::sendPreamble();
		MyProtocol::sendData(command);
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C,typename D>
	static void send(command_t command, const A value_a, const B value_b,
					 const C value_c,const D value_d) {
		buffer_size_t length = 0;
		MyProtocol::startPacket(sizeof(command)+ sizeof(A)+sizeof(B)+sizeof(C)+sizeof(D));
		MyProtocol::sendPreamble();
		MyProtocol::sendData(command);
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		serialize<MyProtocol>(value_d);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C,typename D,typename E>
	static void send(command_t command, const A &value_a, const B &value_b,
					 const C &value_c,const D &value_d,
					 const E &value_e) {
		MyProtocol::startPacket(sizeof(command)+ sizeof(A)+sizeof(B)+sizeof(C)+sizeof(D)+sizeof(E));
		MyProtocol::sendPreamble();
		MyProtocol::sendData(command);
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		serialize<MyProtocol>(value_d);
		serialize<MyProtocol>(value_e);
		MyProtocol::sendPostamble();
	}

	template<typename A,typename B,typename C,typename D,typename E,typename F>
	static void send(command_t command, const A value_a,
					 const B value_b, const C value_c,
					 const D value_d, const E value_e,
					 const F value_f) {
		MyProtocol::startPacket(sizeof(command)+ sizeof(A)+sizeof(B)+sizeof(C)+sizeof(D)+sizeof(E)+sizeof(F));
		MyProtocol::sendPreamble();
		MyProtocol::sendData(command);
		serialize<MyProtocol>(value_a);
		serialize<MyProtocol>(value_b);
		serialize<MyProtocol>(value_c);
		serialize<MyProtocol>(value_d);
		serialize<MyProtocol>(value_e);
		serialize<MyProtocol>(value_f);
		MyProtocol::sendPostamble();
	}

	static inline RawBuffer getRawBuffer() {
		return MyProtocol::getRawBuffer();
	}
};


template<class SerPro, typename A>
struct deserialize {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static A deser(const unsigned char *&b);
};

/* To avoid possible errors with unknown structures, we define
 simple deserialization for POT, and leave above undefined.
 */

template<class SerPro>
struct deserialize<SerPro,uint8_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline uint8_t deser(const unsigned char *&b) {
		uint8_t value = *(uint8_t*)b;
		b+=sizeof(uint8_t);
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,int8_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline int8_t deser(const unsigned char *&b) {
		int8_t value = *(int8_t*)b;
		b+=sizeof(int8_t);
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,uint16_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline uint16_t deser(const unsigned char *&b) {
		uint16_t value = *(uint16_t*)b;
		b+=sizeof(uint16_t);
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,int16_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline int16_t deser(const unsigned char *&b) {
		int8_t value = *(int8_t*)b;
		b+=sizeof(int16_t);
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,int32_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline int32_t deser(const unsigned char *&b) {
		int value = *(int32_t*)b;
		b+=sizeof(int32_t);
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,uint32_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline uint32_t deser(const unsigned char *&b) {
		uint32_t value = *(uint32_t*)b;
		b+=sizeof(uint32_t);
		return value;
	}
};

template<class SerPro, class STRUCT>
struct deserialize<SerPro,const STRUCT*> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static const STRUCT *deser(const unsigned char *&b) {
		STRUCT *r = (STRUCT*)b;
		b+=sizeof(STRUCT);
		return r;
	}
};


template<class SerPro,unsigned int BUFSIZE>
struct deserialize < SerPro, FixedBuffer<BUFSIZE> > {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static FixedBuffer<BUFSIZE> deser(const unsigned char *&b) {
		FixedBuffer<BUFSIZE> buf;
		buf.buffer=(unsigned char*)b;
		b+=BUFSIZE;
		return buf;
	}
};

template<class SerPro, class Function, typename A>
struct deserializer {
	typedef typename SerPro::buffer_size_t buffer_size_t;
};

template<class SerPro, class Function>
struct deserializer<SerPro, Function, void ()> {
	typedef typename SerPro::buffer_size_t buffer_size_t;

	static inline void handle(const unsigned char *&b) {
		Function::handle();
	}
};

template<class SerPro, class Function, typename A>
struct deserializer<SerPro, Function, void (A)> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline void handle(const unsigned char *&b) {
		A val_a=deserialize<SerPro,A>::deser(b);
		Function::handle(val_a);
	}
};


template<class SerPro, class Function,typename A,typename B>
struct deserializer<SerPro, Function, void (A,B)> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline void handle(const unsigned char *&b) {
		A val_a=deserialize<SerPro,A>::deser(b);
		B val_b=deserialize<SerPro,B>::deser(b);
		Function::handle(val_a,val_b);
	}
};

template<class SerPro, class Function, typename A,typename B, typename C>
struct deserializer<SerPro, Function, void (A,B,C)> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline void handle(const unsigned char *&b) {
		A val_a=deserialize<SerPro,A>::deser(b);
		B val_b=deserialize<SerPro,B>::deser(b);
		C val_c=deserialize<SerPro,C>::deser(b);
		Function::handle(val_a, val_b, val_c);
	}
};

template<class SerPro, class Function, typename A,typename B, typename C,typename D>
struct deserializer<SerPro, Function, void (A,B,C,D)> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline void handle(const unsigned char *&b) {
		A val_a=deserialize<SerPro,A>::deser(b);
		B val_b=deserialize<SerPro,B>::deser(b);
		C val_c=deserialize<SerPro,C>::deser(b);
		D val_d=deserialize<SerPro,D>::deser(b);
		Function::handle(val_a, val_b, val_c, val_d);
	}
};

template<class SerPro, class Function,typename A,typename B, typename C,typename D,typename E>
struct deserializer<SerPro, Function, void (A,B,C,D,E)> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline void handle(const unsigned char *&b) {
		A val_a=deserialize<SerPro,A>::deser(b);
		B val_b=deserialize<SerPro,B>::deser(b);
		C val_c=deserialize<SerPro,C>::deser(b);
		D val_d=deserialize<SerPro,D>::deser(b);
		E val_e=deserialize<SerPro,E>::deser(b);
		Function::handle(val_a, val_b, val_c, val_d, val_e);
	}
};

template<class SerPro, unsigned int>
struct EmptyHandler {
	static void handle(unsigned char *&b)
	{
	}
};


template<unsigned int>
struct functionHandler {
    static const int defined = 0;
	static void handle(void);
};


#define DECLARE_FUNCTION(x) \
	template<> \
	struct functionHandler<x> { \
    static const int defined = 1; \
	static void handle

#define END_FUNCTION };

#define DEFAULT_FUNCTION(serpro) \
	template<> \
	struct EmptyHandler<serpro,1> { \
	static void handle


#define DECLARE_SERPRO(config,serial,proto,name) \
	typedef protocolImplementation<config,serial,proto> name;

/*\
	template<> \
	struct deserializer<name, class Function, void (const name::RawBuffer &)> { \
	static void handle(const unsigned char *b, name::buffer_size_t &pos) { \
	Function::handle( name::MyProtocol::getRawBuffer() ); \
	} \
	};\
*/
#define DECLARE_SERPRO_WITH_TIMER(config,serial,proto,timer,name) \
	typedef protocolImplementation<config,serial,proto,timer> name;
/*
 \
	template<> \
	struct deserializer<name, void (const name::RawBuffer &)> { \
	static void handle(const unsigned char *b, name::buffer_size_t &pos, void (*func)(const name::RawBuffer &)) { \
	func( name::MyProtocol::getRawBuffer() ); \
	} \
	};\
 */

#define EXPAND_DELIM ,
#define EXPAND_VALUE(NUMBER,MAX) \
	{  \
	functionHandler<MAX-NUMBER>::defined ? \
	(serpro_deserializer_type)&deserializer<SerPro,functionHandler<MAX-NUMBER>,typeof(functionHandler<MAX-NUMBER>::handle)>::handle \
	: \
	(serpro_deserializer_type)&EmptyHandler<SerPro,1>::handle \
	}

#include "preprocessor_table.h"

#define IMPLEMENT_SERPRO(num,name,proto) \
	typedef void(*serpro_deserializer_type)(const unsigned char*, name::buffer_size_t& pos); \
	template<> \
	name::callback const name::callbacks[] = { \
	DO_EXPAND(num) \
	}; \
	IMPLEMENT_PROTOCOL_##proto(name)


#endif
