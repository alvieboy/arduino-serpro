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
template <class Config, class Ser,class Implementation> class Protocol >
struct protocolImplementation
{
	typedef Protocol<Config,Serial,protocolImplementation> MyProtocol;
	/* Forwarded types */
	typedef typename MyProtocol::command_t command_t;
	typedef typename MyProtocol::buffer_size_t buffer_size_t;
	typedef typename MyProtocol::RawBuffer RawBuffer;

	// callback structure. One for each function we handle. We define both
	// deserializer and function to call.

	typedef  void (*func_type)(void);
	typedef  void (*deserialize_func_type)(const unsigned char *, buffer_size_t&, func_type);

	struct callback {
		deserialize_func_type deserialize;
		func_type func;
	};

	static callback const PROGMEM callbacks[Config::maxFunctions];

	struct VariableBuffer{
		const unsigned char *buffer;
		unsigned int size;
		VariableBuffer(const unsigned char *b,int s): buffer(b),size(s) {
		}
	};

	static inline void deferReply()
	{
		MyProtocol::deferReply();
	}

	static inline void callFunction(int index, const unsigned char *data, buffer_size_t size)
	{
		buffer_size_t pos = 0;
#ifdef AVR
		deserialize_func_type deserialize = (deserialize_func_type)pgm_read_word(&callbacks[index].deserialize);
		func_type func = (func_type)pgm_read_word(&callbacks[index].func);
		deserialize(data,pos,func);
#else
		callbacks[index].deserialize(data,pos,callbacks[index].func);
#endif
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
			serialize<MyProtocol,A>(value);
			MyProtocol::sendPostamble();
		};

	template<typename A>
		static void send(command_t command,const RawBuffer &value) {
			MyProtocol::startPacket(value.size + sizeof(command) );
			MyProtocol::sendPreamble();
			MyProtocol::sendData(command);
			MyProtocol::sendData(value.buffer,value.size);
			MyProtocol::sendPostamble();
		};

	template<typename A>
		static void send(command_t command,const VariableBuffer &value) {
			MyProtocol::startPacket(value.size + sizeof(command) );
			MyProtocol::sendPreamble();
			MyProtocol::sendData(command);
			MyProtocol::sendData(value.buffer,value.size);
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
};


template<class SerPro,typename A>
	struct deserialize {
		typedef typename SerPro::buffer_size_t buffer_size_t;
		static A deser(const unsigned char *b, buffer_size_t &pos);
	};

	/* To avoid possible errors with unknown structures, we define
	 simple deserialization for POT, and leave above undefined.
	 */

	template<class SerPro>
		struct deserialize<SerPro,uint8_t> {
			typedef typename SerPro::buffer_size_t buffer_size_t;
			static inline uint8_t deser(const unsigned char *b, buffer_size_t &pos) {
				uint8_t value = *((uint8_t*)&b[pos]);
				pos+=sizeof(uint8_t);
				return value;
			}
		};

	template<class SerPro>
		struct deserialize<SerPro,int8_t> {
			typedef typename SerPro::buffer_size_t buffer_size_t;
			static inline int8_t deser(const unsigned char *b, buffer_size_t &pos) {
				int8_t value = *(int8_t*)&b[pos];
				pos+=sizeof(int8_t);
				return value;
			}
		};

	template<class SerPro>
		struct deserialize<SerPro,uint16_t> {
			typedef typename SerPro::buffer_size_t buffer_size_t;
			static inline uint16_t deser(const unsigned char *b, buffer_size_t &pos) {
				uint16_t value = *(uint16_t*)&b[pos];
				pos+=sizeof(uint16_t);
				return value;
			}
		};

	template<class SerPro>
		struct deserialize<SerPro,int16_t> {
			typedef typename SerPro::buffer_size_t buffer_size_t;
			static inline int16_t deser(const unsigned char *b, buffer_size_t &pos) {
				int8_t value = *(int8_t*)&b[pos];
				pos+=sizeof(int16_t);
				return value;
			}
		};

	template<class SerPro>
		struct deserialize<SerPro,int32_t> {
			typedef typename SerPro::buffer_size_t buffer_size_t;
			static inline int32_t deser(const unsigned char *b, buffer_size_t &pos) {
				int value = *(int*)&b[pos];
				pos+=sizeof(int32_t);
				return value;
			}
		};

	template<class SerPro>
		struct deserialize<SerPro,uint32_t> {
			typedef typename SerPro::buffer_size_t buffer_size_t;
			static inline uint32_t deser(const unsigned char *b, buffer_size_t &pos) {
				uint32_t value = *(int*)&b[pos];
				pos+=sizeof(uint32_t);
				return value;
			}
		};

	template<class SerPro>
		struct deserialize<SerPro,char*> {
			typedef typename SerPro::buffer_size_t buffer_size_t;
			static char* deser(const unsigned char *b, buffer_size_t &pos) {
				char *value = (char*)&b[pos];
				pos+=strlen(value);
				return value;
			}
		};


	template<class SerPro, class STRUCT>
	struct deserialize<SerPro,const STRUCT*> {
		typedef typename SerPro::buffer_size_t buffer_size_t;
			static const STRUCT *deser(const unsigned char *b, buffer_size_t &pos) {
				STRUCT *p = (STRUCT*)b;
				pos+=sizeof(STRUCT);
				return p;
			}
		};


	template<class SerPro,unsigned int BUFSIZE>
	struct deserialize < SerPro, FixedBuffer<BUFSIZE> > {
		typedef typename SerPro::buffer_size_t buffer_size_t;
			static FixedBuffer<BUFSIZE> deser(const unsigned char *b, buffer_size_t &pos) {
				FixedBuffer<BUFSIZE> buf;
				buf.buffer=(unsigned char*)&b[pos];
				pos+=BUFSIZE;
				return buf;
			}
		};

	template<class SerPro, typename B>
	struct deserializer {
		typedef B func_type;
		typedef typename SerPro::buffer_size_t buffer_size_t;
	};

	template<class SerPro>
	struct deserializer<SerPro, void ()> {
		typedef typename SerPro::buffer_size_t buffer_size_t;
			static inline void handle(const unsigned char *,buffer_size_t &pos, void (*func)(void)) {
				func();
			}
		};

	template<class SerPro, typename A>
	struct deserializer<SerPro, void (A)> {
		typedef typename SerPro::buffer_size_t buffer_size_t;
		static inline void handle(const unsigned char *b, buffer_size_t &pos, void (*func)(A)) {
			A val_a=deserialize<SerPro,A>::deser(b,pos);
			func(val_a);
		}
	};


	template<class SerPro, typename A,typename B>
	struct deserializer<SerPro, void (A,B)> {
		typedef typename SerPro::buffer_size_t buffer_size_t;
		static inline void handle(const unsigned char *b, buffer_size_t &pos, void (*func)(A,B)) {
			A val_a=deserialize<SerPro,A>::deser(b,pos);
			B val_b=deserialize<SerPro,B>::deser(b,pos);
			func(val_a,val_b);
		}
	};

	template<class SerPro, typename A,typename B, typename C>
	struct deserializer<SerPro, void (A,B,C)> {
		typedef typename SerPro::buffer_size_t buffer_size_t;
		static inline void handle(const unsigned char *b, buffer_size_t &pos, void (*func)(A, B, C)) {
			A val_a=deserialize<SerPro,A>::deser(b,pos);
			B val_b=deserialize<SerPro,B>::deser(b,pos);
			C val_c=deserialize<SerPro,C>::deser(b,pos);
			func(val_a, val_b, val_c);
		}
	};

	template<class SerPro, typename A,typename B, typename C,typename D>
	struct deserializer<SerPro, void (A,B,C,D)> {
		typedef typename SerPro::buffer_size_t buffer_size_t;
		static inline void handle(const unsigned char *b, buffer_size_t &pos, void (*func)(A, B, C, D)) {
			A val_a=deserialize<SerPro,A>::deser(b,pos);
			B val_b=deserialize<SerPro,B>::deser(b,pos);
			C val_c=deserialize<SerPro,C>::deser(b,pos);
			D val_d=deserialize<SerPro,D>::deser(b,pos);
			func(val_a, val_b, val_c, val_d);
		}
	};

	template<class SerPro, typename A,typename B, typename C,typename D,typename E>
	struct deserializer<SerPro, void (A,B,C,D,E)> {
		typedef typename SerPro::buffer_size_t buffer_size_t;
		static inline void handle(const unsigned char *b, buffer_size_t &pos, void (*func)(A, B, C, D, E)) {
			A val_a=deserialize<SerPro,A>::deser(b,pos);
			B val_b=deserialize<SerPro,B>::deser(b,pos);
			C val_c=deserialize<SerPro,C>::deser(b,pos);
			D val_d=deserialize<SerPro,D>::deser(b,pos);
			E val_e=deserialize<SerPro,E>::deser(b,pos);
			func(val_a, val_b, val_c, val_d, val_e);
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


#define DECLARE_SERPRO(config,serial,proto,name) \
	typedef protocolImplementation<config,serial,proto> name; \
	template<> \
	struct deserializer<name, void (const name::RawBuffer &)> { \
	static void handle(const unsigned char *b, name::buffer_size_t &pos, void (*func)(const name::RawBuffer &)) { \
	func( name::MyProtocol::getRawBuffer() ); \
	} \
	};\


#define EXPAND_DELIM ,
#define EXPAND_VALUE(NUMBER,MAX) \
	{ (serpro_deserializer_type)&deserializer<SerPro,typeof(functionHandler<MAX-NUMBER>::handle)>::handle,(serpro_function_type)&functionHandler<MAX-NUMBER>::handle }

#include "preprocessor_table.h"

#define IMPLEMENT_SERPRO(num,name,proto) \
	typedef void(*serpro_function_type)(void); \
	typedef void(*serpro_deserializer_type)(const unsigned char*, name::buffer_size_t& pos,void(*)(void)); \
	template<> \
	name::callback const name::callbacks[] = { \
	DO_EXPAND(num) \
	}; \
	IMPLEMENT_PROTOCOL_##proto(name)


#endif
