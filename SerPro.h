/*
 SerPro - A serial protocol for arduino intercommunication
 Copyright (C) 2009-2011 Alvaro Lopes <alvieboy@alvie.com>

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

#include <inttypes.h>
#include "Packet.h"
#include "SerProCommon.h"

#ifndef SERPRO_EMBEDDED
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <tr1/unordered_map>

struct replyPacket
{
	size_t size;
	int id;
	unsigned char *buffer;
	replyPacket():size(0),buffer(0){}
	~replyPacket(){
		if (buffer) {
			delete[] (buffer);
		}
	}
	replyPacket(const replyPacket &p) {
		if (p.buffer) {
			buffer=new unsigned char[p.size];
			memcpy(buffer,p.buffer,p.size);
			size=p.size;
		} else {
			buffer=0;
			size=0;
		}
	}

	replyPacket &operator=(const replyPacket &p)
	{
		if (buffer)
			delete[] buffer;

		if (p.buffer) {
			buffer=new unsigned char[p.size];
			memcpy(buffer,p.buffer,p.size);
			size=p.size;
		} else {
			buffer=0;
			size=0;
		}
        return *this;
	}
};

struct replyPacketCompare
{
	size_t operator()(const replyPacket &a, const replyPacket &b) const {
		return a.id == b.id;
	}
};

#endif


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
#ifndef ZPU
#include <iostream>
#endif

#endif



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
	uint16_t reordered = cpu_to_be16(value);
	MyProtocol::sendData((unsigned char*)&reordered,sizeof(uint16_t));
}

template<typename MyProtocol>
static inline void serialize(const uint32_t &value) {
	uint32_t reordered = cpu_to_be32(value);
	MyProtocol::sendData((unsigned char*)&reordered,sizeof(uint32_t));
}

template<typename MyProtocol>
static inline void serialize(int8_t value) {
	MyProtocol::sendData((uint8_t)value);
}

template<typename MyProtocol>
static inline void serialize(const int16_t &value) {
	uint16_t reordered = cpu_to_be16(value);
	MyProtocol::sendData((unsigned char*)&reordered,sizeof(int16_t));
}

template<typename MyProtocol>
static inline void serialize(const int32_t &value) {
	uint32_t reordered = cpu_to_be32(value);
	MyProtocol::sendData((unsigned char*)&reordered,sizeof(int32_t));
}

template<typename MyProtocol>
static inline void serialize(const VariableBuffer &buf) {
	uint32_t reordered = cpu_to_be32(buf.size);
	MyProtocol::sendData((unsigned char*)&reordered,sizeof(uint32_t));
	if(buf.size>0)
		MyProtocol::sendData(buf.buffer, buf.size);
}

template<typename MyProtocol, unsigned int N>
static inline void serialize(const FixedBuffer<N> &buf) {
	MyProtocol::sendData(buf.buffer,N);
}

/* This is pretty much unsafe... */
#if 0
template<typename MyProtocol>
MAYBESTATIC void serialize(const char *string) {
	MyProtocol::sendData(string,strlen(string));
}
#endif

/* Data dumper */
template<unsigned int>
static void Dumper(const unsigned char *buffer,unsigned int size)
{
}

template<class SerPro, unsigned int>
struct functionHandler {
	static const int defined = 0;
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static void call(const typename SerPro::command_t cmd, const unsigned char*,buffer_size_t &size);
};

template<class SerPro, unsigned int a>
struct maxFunctions {
	static const int value = functionHandler<SerPro, a>::defined ?
	a: maxFunctions<SerPro,a-1>::value;
};

template<class SerPro>
struct maxFunctions<SerPro,0> {
	static const int value = 0;
};

template<class SerPro,typename A>
struct deserialize {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static A deser(const unsigned char *b, buffer_size_t &pos) {
		A r = *((A*)&b[pos]);
		pos+=sizeof(A);
		return r;
	}
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
		uint16_t value = b[pos++];
		value<<=8;
		value+=b[pos++];
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,int16_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline int16_t deser(const unsigned char *b, buffer_size_t &pos) {
		uint16_t value = b[pos++];
		value<<=8;
		value+=b[pos++];
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,int32_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline int32_t deser(const unsigned char *b, buffer_size_t &pos) {
		uint32_t value = b[pos++];
		value<<=8;
		value+=b[pos++];
		value<<=8;
		value+=b[pos++];
		value<<=8;
		value+=b[pos++];
		return value;
	}
};

template<class SerPro>
struct deserialize<SerPro,uint32_t> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static inline uint32_t deser(const unsigned char *b, buffer_size_t &pos) {
		uint32_t value = b[pos++];
		value<<=8;
		value+=b[pos++];
		value<<=8;
		value+=b[pos++];
		value<<=8;
		value+=b[pos++];
		return value;
	}
};
#if 0
template<class SerPro>
struct deserialize<SerPro,char*> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static char* deser(const unsigned char *b, buffer_size_t &pos) {
		char *value = (char*)&b[pos];
		pos+=strlen(value);
		return value;
	}
};
#endif


template<class SerPro, class STRUCT>
struct deserialize<SerPro,const STRUCT*> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static const STRUCT *deser(const unsigned char *b, buffer_size_t &pos) {
		STRUCT *r = (STRUCT*)&b[pos];
		pos+=sizeof(STRUCT);
		return r;
	}
};

template<class SerPro,unsigned int BUFSIZE>
struct deserialize < SerPro, FixedBuffer<BUFSIZE> const &> {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static const FixedBuffer<BUFSIZE> deser(const unsigned char *b, buffer_size_t &pos) {
		FixedBuffer<BUFSIZE> buf;
		buf.buffer=(unsigned char*)&b[pos];
		pos+=BUFSIZE;
		return buf;
	}
};

template<class SerPro>
struct deserialize < SerPro, VariableBuffer > {
	typedef typename SerPro::buffer_size_t buffer_size_t;
	static VariableBuffer deser(const unsigned char *b, buffer_size_t &pos) {

		uint32_t size = b[pos++];
		size<<=8;
		size+=b[pos++];
		size<<=8;
		size+=b[pos++];
		size<<=8;
		size+=b[pos++];

		const unsigned char *p;
		p=&b[pos];
		pos+=size;
		return VariableBuffer(p,size);
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
static inline void handleOOB(const unsigned char *buf, int size)
{
}

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
	typedef Serial MySerial;
    typedef Config MyConfig;

	// callback structure. One for each function we handle. We define both
	// deserializer and function to call.

	typedef  void (*callfunc_t)(const command_t cmd, const unsigned char *, buffer_size_t&);

	struct callback {
		callfunc_t func;
	};

	static callback const PROGMEM callbacks[Config::maxFunctions];

	/* Needed stuff for wait reply */
	static bool isWaitingForReply, replyReady;
	static command_t expectedReplyCommand;

	static uint8_t linkFlags;
	/*
	 static unsigned char replyBuf[Config::maxPacketSize];
	 static buffer_size_t replyBufSize;
	 */
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

#ifndef SERPRO_EMBEDDED
	typedef std::tr1::unordered_map<size_t, replyPacket> replyMap_t;
	static replyMap_t replies;
#endif

	static inline void callFunction(command_t index, const unsigned char *data, buffer_size_t size)
	{
		buffer_size_t pos = 0;
#ifdef AVR
		callfunc_t func = (callfunc_t)pgm_read_word(&callbacks[index].func);
		func(index,data,pos);
#else
		if (index>=Config::maxFunctions) {
		    //fprintf(stderr,"SerPro: INDEX function %d out of bounds!!!!\n",index);
		} else {
			if (Config::implementationType == Master) {
				// Put back
#ifndef SERPRO_EMBEDDED
				if (replies.find(index)==replies.end()) {
					replyPacket r;
					r.id=index;
					r.size = size;
					r.buffer = new unsigned char[size];
					memcpy(r.buffer,data,size);
					replies[index] = r;
				} else {
					std::cerr<<"WARNING: duplicate entry found"<<std::endl;
				}
#endif
			} else {
				callbacks[index].func(index,data,pos);
			}
		}
#endif
	}

	/* Only for Master */
#ifndef SERPRO_EMBEDDED

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

#endif

	static inline void processPacket(const unsigned char *buf,
									 buffer_size_t size)
	{
		buffer_size_t sz = size; 
		// Dump facility
		Dumper<1>(buf,size);

		callFunction(buf[0], buf+1, sz-1);
	}

	static inline void processOOB(const unsigned char *buf,buffer_size_t size)
	{
		handleOOB<1>(buf,size);
	}

	static inline void sendOOB(const unsigned char *buf,buffer_size_t size)
	{
		MyProtocol::sendOOB(buf,size);
	}

	static inline void processData(uint8_t bIn)
	{
		MyProtocol::processData(bIn);
	}

	static inline void send(command_t command) {
		if (Config::implementationType==Master) {
#ifndef SERPRO_EMBEDDED
			sendPacket(command);
#endif
		} else {
			MyProtocol::startPacket(sizeof(command));
			MyProtocol::sendPreamble();
			MyProtocol::sendData(command);
			MyProtocol::sendPostamble();
		}
	}

	template<typename A>
		static void send(command_t command, A value) {
			if (Config::implementationType==Master) {
#ifndef SERPRO_EMBEDDED
				sendPacket(command,value);
#endif
			} else {
				MyProtocol::startPacket(sizeof(A)+sizeof(command));
				MyProtocol::sendPreamble();
				MyProtocol::sendData(command);
				serialize<MyProtocol>(value);
				MyProtocol::sendPostamble();
			}
		};

	template<typename A,typename B>
	static void send(command_t command, const A value_a, const B value_b) {
		if (Config::implementationType==Master) {
#ifndef SERPRO_EMBEDDED
			sendPacket(command,value_a,value_b);
#endif
		} else {
			MyProtocol::startPacket(sizeof(command)+sizeof(A)+sizeof(B));
			MyProtocol::sendPreamble();
			MyProtocol::sendData(command);
			serialize<MyProtocol>(value_a);
			serialize<MyProtocol>(value_b);
			MyProtocol::sendPostamble();
		}
	}

	template<typename A,typename B,typename C>
	static void send(command_t command, const A value_a, const B value_b, const C value_c) {
		if (Config::implementationType==Master) {
#ifndef SERPRO_EMBEDDED
			sendPacket(command,value_a,value_b,value_c);
#endif
		} else {
			MyProtocol::startPacket(sizeof(command)+ sizeof(A)+sizeof(B)+sizeof(C));
			MyProtocol::sendPreamble();
			MyProtocol::sendData(command);
			serialize<MyProtocol>(value_a);
			serialize<MyProtocol>(value_b);
			serialize<MyProtocol>(value_c);
			MyProtocol::sendPostamble();
		}
	}

	template<typename A,typename B,typename C,typename D>
	static void send(command_t command, const A value_a, const B value_b,
					 const C value_c,const D value_d) {
		if (Config::implementationType==Master) {
#ifndef SERPRO_EMBEDDED
			sendPacket(command,value_a,value_b,value_c,value_d);
#endif
		} else {
			MyProtocol::startPacket(sizeof(command)+ sizeof(A)+sizeof(B)+sizeof(C)+sizeof(D));
			MyProtocol::sendPreamble();
			MyProtocol::sendData(command);
			serialize<MyProtocol>(value_a);
			serialize<MyProtocol>(value_b);
			serialize<MyProtocol>(value_c);
			serialize<MyProtocol>(value_d);
			MyProtocol::sendPostamble();
		}
	}

	template<typename A,typename B,typename C,typename D,typename E>
	static void send(command_t command, const A &value_a, const B &value_b,
					 const C &value_c,const D &value_d,
					 const E &value_e) {
		if (Config::implementationType==Master) {
#ifndef SERPRO_EMBEDDED
			sendPacket(command,value_a,value_b,value_c,value_d,value_e);
#endif
		} else {
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
	}

	template<typename A,typename B,typename C,typename D,typename E,typename F>
	static void send(command_t command, const A value_a,
					 const B value_b, const C value_c,
					 const D value_d, const E value_e,
					 const F value_f) {
		if (Config::implementationType==Master) {
#ifndef SERPRO_EMBEDDED
			sendPacket(command,value_a,value_b,value_c,value_d,value_e,value_f);
#endif
		} else {
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
	}
	static inline RawBuffer getRawBuffer() {
		return MyProtocol::getRawBuffer();
	}
#ifndef SERPRO_EMBEDDED
	template<typename R> static void wait(command_t index, R &target)
	{
		replyMap_t::iterator reply;
		do {
			Serial::waitEvents(true);
			reply = replies.find(index);
		} while (reply==replies.end());

		if (reply->second.size) {
            buffer_size_t pos=0;
			target = deserialize<protocolImplementation,R>::deser(reply->second.buffer,pos);
		}
		replies.erase(reply);
        /*
		if (replyBuf) {
			buffer_size_t pos=0;
			//Dumper<1>(replyBuf,replyBufSize);
			target = deserialize<protocolImplementation,R>::deser(replyBuf,pos);
			//Dumper<1>((unsigned char*)target,sizeof(target));
		} */
	}

	static void wait(command_t index)
	{
		replyMap_t::iterator reply;
		do {
			Serial::waitEvents(true);
			reply = replies.find(index);
		} while (reply==replies.end());

		/*if (reply->second.size) {
            buffer_size_t pos=0;
			target = deserialize<protocolImplementation,R>::deser(reply->second.buffer,pos);
		} */
		replies.erase(reply);
	}
#ifndef SERPRO_EMBEDDED
	static void initPostLink(uint8_t val){
		// TODO: save peer properties.
		std::cerr<<"Link type "<<val<<std::endl;
		linkFlags = val;
	}

	static void initLayer(); 

#endif

	static uint8_t ____serpro_init_request()
	{
		// TODO: Return proper configuration here
#ifdef ZPU
		return 0x00; // Big endian
#else
        return 0x01; // Litlle endian
#endif
	}
#endif

};



template<class SerPro, unsigned int>
static void nohandler(const typename SerPro::command_t cmd, const unsigned char *b,typename SerPro::buffer_size_t &pos)
{
}

template<class SerPro, unsigned int N, typename A>
struct functionSlot {};


template<class SerPro, unsigned int N>
struct functionSlot<SerPro, N, void(void)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, void (*func)(void)) {
		func();
		SerPro::send(cmd);
	}
};

template<class SerPro, unsigned int N, typename A>
struct functionSlot<SerPro, N, void(A)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, void (*func)(A)) {
		A val_a=deserialize<SerPro,A>::deser(b,pos);
		func(val_a);
		SerPro::send(cmd);
	}
};

template<class SerPro, unsigned int N, typename R, typename A>
struct functionSlot<SerPro, N, R (A)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, R (*func)(A)) {
		A val_a=deserialize<SerPro,A>::deser(b,pos);
		R ret = func(val_a);
		SerPro::send(cmd,ret);
	}
};

template<class SerPro, unsigned int N, typename R>
struct functionSlot<SerPro, N, R (void)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, R (*func)(void)) {
		R ret = func();
		SerPro::send(cmd,ret);
	}
};

template<class SerPro, unsigned int N, typename R, typename A, typename B>
struct functionSlot<SerPro, N, R (A,B)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, R (*func)(A,B)) {
		A val_a=deserialize<SerPro,A>::deser(b,pos);
		B val_b=deserialize<SerPro,B>::deser(b,pos);
		R ret = func(val_a,val_b);
		SerPro::send(cmd,ret);
	}
};

template<class SerPro, unsigned int N, typename A, typename B>
struct functionSlot<SerPro, N, void (A,B)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, void (*func)(A,B)) {
		A val_a=deserialize<SerPro,A>::deser(b,pos);
		B val_b=deserialize<SerPro,B>::deser(b,pos);
		func(val_a,val_b);
		SerPro::send(cmd);
	}
};

template<class SerPro, unsigned int N, typename R, typename A, typename B,typename C>
struct functionSlot<SerPro, N, R (A,B,C)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, R (*func)(A,B,C)) {
		A val_a=deserialize<SerPro,A>::deser(b,pos);
		B val_b=deserialize<SerPro,B>::deser(b,pos);
		C val_c=deserialize<SerPro,C>::deser(b,pos);
		R ret = func(val_a,val_b,val_c);
		SerPro::send(cmd,ret);
	}
};

template<class SerPro, unsigned int N, typename A, typename B,typename C>
struct functionSlot<SerPro, N, void (A,B,C)> {

	typedef typename SerPro::buffer_size_t buffer_size_t;

	static void call(const typename SerPro::command_t cmd, const unsigned char *b,buffer_size_t &pos, void (*func)(A,B,C)) {
		A val_a=deserialize<SerPro,A>::deser(b,pos);
		B val_b=deserialize<SerPro,B>::deser(b,pos);
		C val_c=deserialize<SerPro,C>::deser(b,pos);
		func(val_a,val_b,val_c);
		SerPro::send(cmd);
	}
};

typedef enum {
	SERPRO_LINK_UP,
	SERPRO_LINK_DOWN
} serpro_event_type;

template<serpro_event_type t>
static inline void handleEvent() {
};

#define EXPORT_FUNCTION(n, name) \
	template<class SerPro> \
	struct functionHandler<SerPro, n> { \
	static const int defined = 1; \
	static const struct functionSlot<SerPro,n,typeof name> slot; \
	static void call(const typename SerPro::command_t cmd, const unsigned char *b,typename SerPro::buffer_size_t &pos) { slot.call(cmd,b,pos,name); } \
	}

#define END_FUNCTION };

#define DEFAULT_FUNCTION \
	template<class SerPro> \
	void nohandler<SerPro,1>(const unsigned char *b,typename SerPro::buffer_size_t &pos)

#ifndef SERPRO_EMBEDDED
#define SERPRO_EXTRADEFS(name) \
	template<> name::replyMap_t name::replies=name::replyMap_t();


#else
#define SERPRO_EXTRADEFS(name)
#endif

#define DECLARE_SERPRO(config,serial,proto,name) \
	typedef protocolImplementation<config,serial,proto> name; \
	template<> \
	struct deserializer<name, void (const name::RawBuffer &)> { \
	static void handle(const unsigned char *b, name::buffer_size_t &pos, void (*func)(const name::RawBuffer &)) { \
	func( name::MyProtocol::getRawBuffer() ); \
	} \
	};\
    SERPRO_EXTRADEFS(name)

#define DECLARE_SERPRO_WITH_TIMER(config,serial,proto,timer,name) \
	typedef protocolImplementation<config,serial,proto,timer> name; \
	template<> \
	struct deserializer<name, void (const name::RawBuffer &)> { \
	static void handle(const unsigned char *b, name::buffer_size_t &pos, void (*func)(const name::RawBuffer &)) { \
	func( name::MyProtocol::getRawBuffer() ); \
	} \
	};\
    SERPRO_EXTRADEFS(name)


#define EXPAND_DELIM ,
#define EXPAND_VALUE(NUMBER,MAX) \
	{ functionHandler<SerPro,MAX-NUMBER>::defined ? (callfunc_t)&functionHandler<SerPro,MAX-NUMBER>::call : (callfunc_t)&nohandler<SerPro,1> }

#include "preprocessor_table.h"

#ifndef SERPRO_EMBEDDED
#define SERPRO_IMPL_EXTRADEFS(name) \
	IMPORT_FUNCTION(0,_____serpro_initialize,uint8_t(void)); \
    \
	template<> void name::initLayer() { \
		std::cerr<<"Initializing"<<std::endl; \
		initPostLink(_____serpro_initialize());  \
		std::cerr<<"LINK UP"<<std::endl; \
	}

#else
#define SERPRO_IMPL_EXTRADEFS(name) \
	EXPORT_FUNCTION(0,name::_____serpro_init_request);
#endif

#define IMPLEMENT_SERPRO(num,name,proto) \
	typedef void(*serpro_function_type)(void); \
	typedef void(*serpro_deserializer_type)(const unsigned char*, name::buffer_size_t& pos,void(*)(void)); \
	template<> uint8_t name::linkFlags=0; \
	template<> \
	name::callback const name::callbacks[] = { \
	DO_EXPAND(num) \
	}; \
	SERPRO_IMPL_EXTRADEFS(name) \
	IMPLEMENT_PROTOCOL_##proto(name) \



#define SERPRO_EVENT(event) template<> void LL_handleEvent<event>()

template<class SerPro,unsigned int N, typename A>
struct CallSlot {
};

template<class SerPro,unsigned int N, typename A>
struct CallSlot< SerPro, N, void (A) > {
	void operator()(A a) {
		SerPro::send(N, a);
		SerPro::wait(N);
	}
};

template<class SerPro,unsigned int N>
struct CallSlot< SerPro, N, void (void) > {
	void operator()() {
		SerPro::send(N);
		SerPro::wait(N);
	}
};

template<class SerPro,unsigned int N, typename R>
struct CallSlot<SerPro, N, R (void) > {
	R operator()() {
		SerPro::send(N);
		R ret;
		SerPro::wait(N,ret);
		return ret;
	}
};

template<class SerPro,unsigned int N, typename A, typename R>
struct CallSlot<SerPro, N, R (A) > {
	R operator()(A a) {
		SerPro::send(N, a);
		R ret;
		SerPro::wait(N,ret);
		return ret;
	}
};

template<class SerPro,unsigned int N, typename A, typename R, typename B>
struct CallSlot<SerPro, N, R (A,B) > {
	R operator()(A a, B b) {
		SerPro::send(N, a, b);
		R ret;
		SerPro::wait(N,ret);
		return ret;
	}
};

template<class SerPro,unsigned int N, typename A, typename B>
struct CallSlot<SerPro, N, void (A,B) > {
	void operator()(A a, B b) {
		SerPro::send(N, a, b);
		SerPro::wait(N);
	}
};

template<class SerPro,unsigned int N, typename A, typename R, typename B,typename C>
struct CallSlot<SerPro, N, R (A,B,C) > {
	R operator()(A a, B b, C c) {
		SerPro::send(N, a, b, c);
		R ret;
		SerPro::wait(N,ret);
		return ret;
	}
};

template<class SerPro,unsigned int N, typename A, typename B, typename C>
struct CallSlot<SerPro, N, void (A,B,C) > {
	void operator()(A a, B b, C c) {
		SerPro::send(N, a, b, c);
		SerPro::wait(N);
	}
};

#define IMPORT_FUNCTION(index, name, type) \
    static CallSlot<SerPro,index, type> name;


#endif
