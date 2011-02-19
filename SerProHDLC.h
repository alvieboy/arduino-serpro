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

/*

Implementation according to ISO4335 extracted from:
http://www.acacia-net.com/wwwcla/protocol/iso_4335.htm

*/

#ifndef __SERPRO_HDLC__
#define __SERPRO_HDLC__

#include <inttypes.h>

#ifndef SERPROLIBRARY
#include "crc16.h"
#include "SerProCommon.h"
#include "Packet.h"
#else
#include <serpro/crc16.h>
#include <serpro/SerProCommon.h>
#include <serpro/Packet.h>
#endif

#ifndef SERPRO_EMBEDDED

#include <bits/endian.h>
#include <queue>
#include <unistd.h>

#else

#define __LITTLE_ENDIAN 0
#define __BIG_ENDIAN 1


#if defined(AVR)
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#if defined(ZPU)
#define __BYTE_ORDER __BIG_ENDIAN
#endif

#endif

#ifndef SERPRO_EMBEDDED
#include <stdio.h>
#ifdef SERPRO_DEBUG
#define LOG(m...) fprintf(stderr,"SerProHDLC [%d] ",getpid()); fprintf(stderr,m);
#define LOGN(m...)  fprintf(stderr,m);
#else
#define LOG(m...)
#define LOGN(m...)
#endif
#else
#define LOG(m...)
#define LOGN(m...)
#endif


static uint8_t const HDLC_frameFlag = 0x7E;
static uint8_t const HDLC_escapeFlag = 0x7D;
static uint8_t const HDLC_escapeXOR = 0x20;

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


/* Packet definition  - not to be used on Arduino */
#ifndef SERPRO_EMBEDDED

template<class Config, class Serial>
class HDLCPacket: public Packet
{
	CRC16_ccitt outcrc;
	uint8_t outCommand;

	void sendRAW(uint8_t b)
	{
		Serial::write(b);
	}


public:

	void startPacket(uint8_t command, int payload_size) {
		outcrc.reset();
		outCommand=command;
	}

	void sendByte(uint8_t b)
	{
		if (b==HDLC_frameFlag || b==HDLC_escapeFlag) {
			sendRAW(HDLC_escapeFlag);
			sendRAW(b ^ HDLC_escapeXOR);
		} else
			sendRAW(b);
	}

	void sendData(const uint8_t *buf, size_t size)
	{
		for(;size>0;size--,buf++) {
			sendData(*buf);
		}
	}

	void sendData(uint8_t b)
	{
		outcrc.update(b);
		sendByte(b);
	}

	HDLCPacket() {
		payload_size=0;
	}

	void append(const uint8_t &b) {
		//LOG("Packet add: %02x\n",b);
		payload[payload_size++] = b;
	}

	void appendBuffer(const uint8_t *buf, size_t size)
	{
		for(;size>0;size--,buf++) {
			append(*buf);
		}
	}

	int getSize() {
		return payload_size;
	}

	void send(uint8_t control) {
		outcrc.reset();
		sendPreamble(control);
		if (payload_size>0)
			sendData(payload,payload_size);
		sendPostamble();
	}


	void sendPreamble(uint8_t control) {
		sendRAW(HDLC_frameFlag);
		sendByte(0xff); // Target
		outcrc.update(0xff);
		sendByte(control);
		outcrc.update(control);
	}

	void sendPostamble() {
		uint16_t crc = outcrc.get();
		sendByte((crc>>8)&0xff);
		sendByte(crc & 0xff);
		sendRAW(HDLC_frameFlag);
		Serial::flush();
	}

	void setSeq(int s){
		seq=s;
	}
	int getSeq() {
		return seq;
	}

	void append(const uint16_t &value) {
		append((uint8_t)((value>>8)&0xff));
		append((uint8_t)(value&0xff));
	}

	void append(const uint32_t &value) {
		append((uint8_t)(value>>24 &0xff));
		append((uint8_t)(value>>16 &0xff));
		append((uint8_t)(value>>8 &0xff));
		append((uint8_t)(value &0xff));
	}
	void append(const int8_t &b) {
		append((uint8_t)b);
	}
	void append(const int16_t &b) {
		append((uint16_t)b);
	}
	void append(const int32_t &b) {
		append((uint32_t)b);
	}

	uint8_t payload[Config::maxPacketSize];
	int payload_size;
	int seq;
};


class PacketQueue
{
public:

	PacketQueue() {
		ack=0;
		tx=0;
		int i;
		for (i=0;i<7;i++)
			slots[i]=NULL;
	};

	uint8_t toBeAcked() {
		//LOG("Packets to be acked: %d\n",(tx-ack)&0x7);
		return (tx - ack) & 0x7;
	}

	Packet *getByID(uint8_t) const;

	uint8_t queue(Packet *p) {
		if (((tx+1)&7)==ack)
			return -1;

		LOG("Queuing TX packet %p at %d\n",p,tx);
		slots[tx] = p;
		tx++;
		LOG("TXa %u\n",tx);
		tx = tx & 0x7;
		LOG("Next TX pos: %u\n",tx);
		return 0;
	}

	Packet *dequeue() {
		if (toBeAcked()>0) {
			Packet *r = slots[ack++];
			ack &= 0x7;
			return r;
		} else {
			return NULL;
		}
	}
	uint8_t peekIndex() {
		return (tx-1)&0x7;
	}
	Packet *peek()
	{
		LOG("PEEK: ack %u, tx %u\n",ack,tx);
		if (toBeAcked()>0) {
			Packet *r = slots[(tx-1)&0x7];
			LOG("PEEK packet: %p (index %u)\n",r,(tx-1)&0x7);
			return r;
		} else {
			return NULL;
		}
	}
	void ackUpTo(uint8_t num)
	{
		LOG("ACK'ing up to sequence %u\n",num);
		uint8_t i = ack;
		while (i!=num) {
			if (slots[i]) {
				/* Remove */
				LOG("Acking %u (%p) and freeing\n",i,slots[i]);
				delete(slots[i]);
				slots[i]=NULL;
			} else {
				LOG("Ack'ing non ackable frame %u\n",i);
			}
			ack=num;
			i++;
			i&=7;
		}
		LOG("ACK counter now %u\n",ack);
	}

private:
	Packet *slots[8];
	uint8_t tx,ack;
};

#endif

typedef enum {
	LINK_UP,
	LINK_DOWN,
	CRC_ERROR,
	EMPTY_FRAME,
	START_XMIT,
	END_XMIT,
	START_FRAME,
	END_FRAME
} event_type;

template<event_type t>
static inline void LL_handleEvent() {
}

typedef enum {
	FULL,      /* Full implementation */
	BASIC,     /* Basic (arduino) implementation */
	MINIMAL    /* Minimal */
} HDLCImplementationType;

struct HDLC_DefaultConfig {
	typedef CRC16_ccitt CRC;
	static const HDLCImplementationType implementationType = BASIC;
	static const unsigned int linkTimeout = 1000;
	static const unsigned int retransmitTimeout = 1000;
};

template<class Config,class Serial,class Implementation,class Timer> class SerProHDLC
{
public:
	/* Buffer */
	static unsigned char _pBuf_internal[Config::maxPacketSize];
	static unsigned char *getBuffer() { return _pBuf_internal+1; } // Enforce alignment

	typedef typename Config::HDLC::CRC CRCTYPE;

	typedef typename CRCTYPE::crc_t crc_t;

	static CRCTYPE crcgen; // Only one CRC generator. We cannot TX and RX data on slavemode

	// Queued CRC.
	static crc_t crcSaveA, crcSaveB;

	typedef uint8_t command_t;

#ifdef AVR
	typedef uint8_t cpuword_t;
#else
    typedef uint32_t cpuword_t;
#endif

#ifndef SERPRO_EMBEDDED
	typedef HDLCPacket<Config,Serial> MyPacket;
	static PacketQueue txQueue, rxQueue;
	static std::queue<Packet*> inputQueue;
	static bool linkReady;
#endif
#ifdef AVR
	typedef typename best_storage_class< number_of_bytes<Config::maxPacketSize>::bytes >::type buffer_size_t;
#else
    typedef unsigned int buffer_size_t;
#endif
	//typedef uint16_t buffer_size_t;
	typedef uint16_t packet_size_t;

	static buffer_size_t pBufPtr;
	static packet_size_t pSize,lastPacketSize;

	/* HDLC parameters extracted from frame */
	static cpuword_t inAddressField;
	static cpuword_t inControlField;
	static cpuword_t allowedPacketsInTransit;

	/* HDLC control data */
	static cpuword_t txSeqNum;        // Transmit sequence number
	static cpuword_t rxNextSeqNum;    // Expected receive sequence number

	static cpuword_t unEscaping;
	static cpuword_t inPacket;

	struct RawBuffer {
		unsigned char *buffer;
		buffer_size_t size;
	};

	static cpuword_t linkFlags;

#define LINK_FLAG_LINKUP 1
#define LINK_FLAG_PACKETSENT 2

	struct HDLC_header {
		uint8_t address;
		union {
			struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
				uint8_t flag: 1;   /* Always zero */
				uint8_t txseq: 3;  /* TX sequence number */
				uint8_t poll: 1;   /* poll/final */
				uint8_t rxseq: 3;  /* RX sequence number */
#elif __BYTE_ORDER == __BIG_ENDIAN
				uint8_t rxseq: 3;  /* RX sequence number */
				uint8_t poll: 1;   /* poll/final */
				uint8_t txseq: 3;  /* TX sequence number */
				uint8_t flag: 1;   /* Always zero */
#else
# error "No endianess defined."
#endif
			} iframe;

			struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
				uint8_t flag: 2;   /* '1''0' for S-frame */
				uint8_t function: 2; /* Supervisory function */
				uint8_t poll: 1;     /* Poll/Final */
				uint8_t seq: 3;    /* Sequence number */
#elif __BYTE_ORDER == __BIG_ENDIAN
				uint8_t seq: 3;    /* Sequence number */
				uint8_t poll: 1;     /* Poll/Final */
				uint8_t function: 2; /* Supervisory function */
				uint8_t flag: 2;   /* '1''0' for S-frame */
#else
# error "No endianess defined."
#endif
			} sframe;

			struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
				uint8_t flag: 2;   /* '1''1' for U-Frame */
				uint8_t modifier: 2; /* Modifier bits */
				uint8_t poll: 1;   /* Poll/Final */
				uint8_t function: 3; /* Function */
#elif __BYTE_ORDER == __BIG_ENDIAN
				uint8_t function: 3; /* Function */
				uint8_t poll: 1;   /* Poll/Final */
				uint8_t modifier: 2; /* Modifier bits */
				uint8_t flag: 2;   /* '1''1' for U-Frame */
#else
# error "No endianess defined."
#endif
			} uframe;

			struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
				uint8_t flag: 2;  /* LSB-MSB:  0X: I-frame, 10: S-Frame, 11: U-Frame */
				uint8_t unused: 6;
#elif __BYTE_ORDER == __BIG_ENDIAN
				uint8_t unused: 6;
				uint8_t flag: 2;  /* LSB-MSB:  0X: I-frame, 10: S-Frame, 11: U-Frame */
#else
# error "No endianess defined."
#endif
			} frame_type;
			uint8_t value;
		} control;
	};


	static inline void dumpPacket() { /* Debuggin only */
#if 0
		unsigned i;
		LOG("Packet: %d bytes\n", lastPacketSize);
		LOG("Dump (hex): ");
		for (i=0; i<lastPacketSize; i++) {
		LOGN("0x%02X ", (unsigned)pBuf[i+2]);
		}
		LOGN("\n");
#endif
	}
	static inline RawBuffer getRawBuffer()
	{
		RawBuffer r;
		r.buffer = getBuffer()+2;
		r.size = lastPacketSize;
		LOG("getRawBuffer() : size %u %u\n", r.size,lastPacketSize);
		return r;
	}

	static void sendByte(uint8_t byte);

	/* Frame types */
	enum frame_type {
		FRAME_INFORMATION,
		FRAME_SUPERVISORY,
		FRAME_UNNUMBERED
	};


	/* Supervisory commands */
	enum supervisory_command {
		RR  =  0x0,   // Receiver ready
		RNR =  0x1,   // Receiver not ready
		REJ =  0x2,   // Reject
		SREJ = 0x3    // Selective-reject
	};


	/* This field is actually a combination of
	 modifier plus function type. We number then
	 according to concatenation of both.

	 Bits: MSB->LSB
	 MMMUMM11
	 */
	enum unnumbered_command {
		UI          = 0x00, // 00-000 Unnumbered Information
		SNRM        = 0x80, // 00-001 Set Normal Response Mode
		RD          = 0x40, // 00-010 Request Disconnect
		// Not used 00-011 - Reused for arduino extension
		SNURM       = 0xC0, // 00-011 Set Normal Unbuffered Response Mode
		UP          = 0x20, // 00-100 Unnumbered Poll
		// Not used 00-101
		UA          = 0x60, // 00-110 Unnumbered Acknowledgment
		TEST        = 0xE0, // 00-111 Test
		// All 01-XXX are not used
		RIM         = 0x04, // 10-000 Request Initialization Mode
		FRMR        = 0x24, // 10-001 Frame Reject
		SIM         = 0x44, // 10-010 Set Initialization Mode
		// Not used any other 10-XXX
		DM          = 0x0C, // 11-000 Disconnected Mode
		RSET        = 0x8C, // 11-001 Reset
		SARME       = 0x4C, // 11-010 Set Asynchronous Response Mode Extended
		SNRME       = 0xCC, // 11-011 Set Normal Response Mode Extended
		SARM        = 0x2C, // 11-100 Set Asynchronous Response Mode
		// SABM        // 11-101 Set Asynchronous Balanced Mode DUP
		XID         = 0xAC, // 11-101 Exchange identification
		SABME       = 0x6C  // 11-110 Set Asynchronous Balanced Mode Extended
		// 11-111 Not used
	};

	typedef typename Timer::timer_t timer_t;
	static timer_t linktimer;
	static timer_t retransmittimer;

	static int linkExpired(void*d)
	{
		if (Config::HDLC::implementationType == BASIC ||
			Config::HDLC::implementationType == FULL ) {
			LOG("Link timeout, retrying\n");
			startLink();
		}
		return 0;
	}

	static int retransmitTimerExpired(void*d)
	{
		if (Config::HDLC::implementationType == BASIC ||
			Config::HDLC::implementationType == FULL ) {

			LOG("TX timeout\n");
			if (isLinkUp()) {
				startXmit();
			}
		}
		return 0;
	}

	static void startLink()
	{
		if (Config::HDLC::implementationType == BASIC ||
			Config::HDLC::implementationType == FULL ) {

			if (Timer::defined(linktimer)) {
				linktimer = Timer::cancelTimer(linktimer);
			}
			linktimer = Timer::addTimer( &linkExpired, Config::HDLC::linkTimeout);
			sendUnnumberedFrame( SNURM );
		}
	}

	static inline void sendInformationControlField()
	{
		uint8_t ifield;
		ifield = txSeqNum<<1 | rxNextSeqNum<<5 | 0x10;
		sendByte( ifield );
		crcgen.update( ifield );
	}

	static void startPacket(packet_size_t len)
	{
		crcgen.reset();
		LL_handleEvent<START_XMIT>();
		pBufPtr=0;
	}

	static void startXmit()
	{
#ifndef SERPRO_EMBEDDED
		if (Config::implementationType==Master) {
			HDLCPacket<Config,Serial> *p = static_cast<HDLCPacket<Config,Serial>*>(txQueue.peek());
			if (p) {
				// Compute control field.
				uint8_t control;
				control = (txQueue.peekIndex())<<1;
				control |= 0x10; // Poll
				control |= (rxNextSeqNum<<5);
				p->send(control);
				if (Timer::defined(retransmittimer)) {
					retransmittimer = Timer::cancelTimer(retransmittimer);
				}
				retransmittimer = Timer::addTimer( &retransmitTimerExpired, Config::HDLC::retransmitTimeout);
			}
		}
#endif
	}

#ifndef SERPRO_EMBEDDED
	static int getQueueSize()
	{
		return inputQueue.size();
	}
#endif
	static void checkXmit()
	{
#ifndef SERPRO_EMBEDDED
		if (Config::implementationType==Master) {
			LOG("Checking Xmit queue...\n");
			if (txQueue.toBeAcked()<allowedPacketsInTransit && inputQueue.size()>0) {
				Packet *p = inputQueue.front();
				inputQueue.pop();
				LOG("Queuing packet for transmission %p\n",p);
				queueTransmit(p);
			}
		}
#endif
	}

#ifndef SERPRO_EMBEDDED
	static int queueTransmit(Packet *p)
	{
		if (Config::implementationType==Master) {
			if (txQueue.toBeAcked() < allowedPacketsInTransit) {
				txQueue.queue( p );
				startXmit();
			} else {
				inputQueue.push(p);
			}
			return 1;
		}
		return 0;
	}
#endif

	static void sendPreamble();

	static void sendPreamble(uint8_t control)
	{
		Serial::write( HDLC_frameFlag );
		sendByte( (uint8_t)Config::HDLC::stationId );
		crcgen.update( (uint8_t)Config::HDLC::stationId );
		sendByte(control);
		crcgen.update(control);
	}

	static void sendPostamble();
	static void sendSUPostamble()
	{
		CRC16_ccitt::crc_t crc = crcgen.get();
		sendByte(crc>>8);
		sendByte(crc & 0xff);
		Serial::write(HDLC_frameFlag);
		Serial::flush();
		LL_handleEvent<END_XMIT>();
	}

	static void sendData(const unsigned char *buf, packet_size_t size)
	{
		//packet_size_t i;
		//LOG("Sending %d payload\n",size);
		while(size--) {
			crcgen.update(*buf);
			sendByte(*buf++);
		}
		/*
		for (i=0;i<size;i++) {
			crcgen.update(buf[i]);
			sendByte(buf[i]);
		} */
	}

	static void sendData(unsigned char c)
	{
		crcgen.update(c);
		sendByte(c);
	}

	static void sendCommandPacket(command_t const command, unsigned char * const buf, packet_size_t const size)
	{
		startPacket(size);
		sendPreamble();
		crcgen.update( command );
		sendData(command);
		sendData(buf,size);
		sendPostamble();
	}

	static inline void deferReply()
	{
		linkFlags |= LINK_FLAG_PACKETSENT;
	}
#ifndef SERPRO_EMBEDDED
	static Packet *createPacket()
	{
		Packet * p =new MyPacket();
		LOG("Created packet at %p\n",p);
		return p;
	}
#endif

	static void handle_supervisory()
	{
		if (Config::HDLC::implementationType!=MINIMAL) {
			LOG("Got supervisory frame\n");
			HDLC_header *h = (HDLC_header*)getBuffer();
			supervisory_command c = (supervisory_command)(h->control.sframe.function);
			LOG("Got supervisory frame 0x%02x\n", c);
			switch (c) {
			case RR:
				LOG("RR, ack'ed 0x%02x\n", h->control.sframe.seq);
#ifndef SERPRO_EMBEDDED
				if (Config::implementationType==Master) {

					txQueue.ackUpTo(h->control.sframe.seq);

					if (Timer::defined(retransmittimer) && txQueue.toBeAcked()==0) {
						retransmittimer = Timer::cancelTimer(retransmittimer);
					}

					checkXmit();
				}
				linkReady = true;
#endif
				break;
			case REJ:
				if (Config::implementationType==Master) {
					LOG("Got REJ for sequence %u\n",h->control.sframe.seq);
					break;
				}
#ifndef SERPRO_EMBEDDED
			case RNR:
				linkReady = false;
				break;
#endif
			default:
				LOG("Unhandled supervisory frame\n");
			}
		}
	}

	static inline bool isLinkUp() {
		return !!(linkFlags & LINK_FLAG_LINKUP);
	}

	static void setLinkUP()
	{
		linkFlags |= LINK_FLAG_LINKUP;
		LL_handleEvent<LINK_UP>();
		checkXmit();
	}
	static void setLinkDOWN()
	{
		linkFlags &= ~LINK_FLAG_LINKUP;
		LL_handleEvent<LINK_DOWN>();
	}

	static void handle_unnumbered()
	{
		HDLC_header *h = (HDLC_header*)getBuffer();
		unnumbered_command c = (unnumbered_command)(h->control.value & 0xEC);
		LOG("Unnumbered frame 0x%02x (0x%02x)\n",c,h->control.value);
		switch(c) {
		case SNURM:
			if (Config::HDLC::implementationType == BASIC ||
				Config::HDLC::implementationType == FULL ) {

				// Reset tx/rx sequences
				txSeqNum=0;
				rxNextSeqNum=0;

				sendUnnumberedFrame(UA);
				LOG("Link up, NRM\n");
				setLinkUP();
			}
			break;
		case DM:
			if (Config::HDLC::implementationType == BASIC ||
				Config::HDLC::implementationType == FULL ) {
				setLinkDOWN();
				LOG("Link down\n");
			}
			break;
		case UA:
			if (Config::HDLC::implementationType == BASIC ||
				Config::HDLC::implementationType == FULL ) {

				// Reset tx/rx sequences
				txSeqNum=0;
				rxNextSeqNum=0;

				LOG("Link up, by our request\n");
				if (Config::implementationType==Master) {
					if (Timer::defined(linktimer))
						linktimer = Timer::cancelTimer(linktimer);
				}
				setLinkUP();

			}
			break;
		case UI:
			if (Config::HDLC::implementationType == MINIMAL) {
				Implementation::processPacket(getBuffer()+2,pBufPtr-4);
			} else {
				Implementation::processOOB(getBuffer()+2,pBufPtr-4);
			}
			break;
		default:
			if (Config::HDLC::implementationType == BASIC ||
				Config::HDLC::implementationType == FULL ) {
				sendUnnumberedFrame(DM);
				setLinkDOWN();
				LOG("Link down\n");
			}
			break;
		}
	}

	static void sendOOB(const unsigned char *buf, unsigned int size)
	{
		startPacket(size);
		Serial::write( HDLC_frameFlag );
		sendByte( (uint8_t)Config::HDLC::stationId );
		crcgen.update( (uint8_t)Config::HDLC::stationId );
		sendByte( 0x03 | UI );
		crcgen.update( 0x03 | UI);
		sendData(buf,size);
		sendSUPostamble();
	}

	static void sendUnnumberedFrame(unnumbered_command c)
	{
		uint8_t v = (uint8_t)c;
		v |= 0x03;

		startPacket(0);
		Serial::write( HDLC_frameFlag );
		sendByte( (uint8_t)Config::HDLC::stationId );
		crcgen.update( (uint8_t)Config::HDLC::stationId );
		sendByte(v);
		crcgen.update(v);
		sendSUPostamble();
	}

	static void sendSupervisoryFrame(supervisory_command c);

	static void ackLastFrame()
	{
		if (Config::implementationType == Slave) {
			uint8_t v = RR << 2;
			v|= 0x01;
			v|= (rxNextSeqNum & 0x7)<<5;

			LOG("Acknowledging last frame 0x%02x\n",v);

			startPacket(0);
			Serial::write( HDLC_frameFlag );
			sendByte( (uint8_t)Config::HDLC::stationId );
			crcgen.update( (uint8_t)Config::HDLC::stationId );
			sendByte(v);
			crcgen.update(v);
			sendSUPostamble();
		}
	}

	static void handleInformationFrame(const HDLC_header *h)
	{
#ifndef SERPRO_EMBEDDED
		if (Config::implementationType==Master) {
			/* Ack frames */
			txQueue.ackUpTo(h->control.iframe.rxseq);
			checkXmit();
		}
#endif
	}

	static __attribute__((noinline)) void preProcessPacket()
	{
		HDLC_header *h = (HDLC_header*)getBuffer();
		/* Check CRC */
		if (pBufPtr<4) {
			/* Empty/erroneous packet */
			LOG("Short packet received, len %u\n",pBufPtr);
			return;
		}
		/* Make sure packet is meant for us. We can safely check
		 this before actually computing CRC */

		packet_size_t i = pBufPtr-2;
        /*
		incrc.reset();
		for (i=0;i<pBufPtr-2;i++) {
			incrc.update(pBuf[i]);
			}
			*/
		crc_t pcrc = getBuffer()[i++];
		pcrc<<=8;
		pcrc|=getBuffer()[i];
		if (pcrc!=crcgen.getMinusTwo()) {
			/* CRC error */
			LOG("CRC ERROR, expected 0x%04x, got 0x%04x offset %d\n",
				(unsigned)crcgen.getMinusTwo(),(unsigned)pcrc,
				i);
#if 1
			{
				unsigned int mn=crcgen.getMinusTwo();
				mn<<=16;
				mn+=pcrc;

				sendOOB((unsigned char*)&mn,sizeof(mn));
			}
#endif
			LL_handleEvent<CRC_ERROR>();
			return;
		}
		LOG("CRC MATCH 0x%04x, got 0x%04x\n",crcgen.getMinusTwo(),pcrc);
		lastPacketSize = pBufPtr-4;
		LOG("Packet details: destination ID %u, control 0x%02x, size %d\n",
			h->address,h->control.value,
			(unsigned)lastPacketSize);
		dumpPacket();
		if ((h->control.frame_type.flag & 1) == 0) {
			/* Information  */
			LOG("Information frame\n");

			/* Ensure this packet comes in sequence */

			if (rxNextSeqNum != h->control.iframe.txseq) {
				// Reject frame here ????
				sendSupervisoryFrame(REJ);

				LOG("************* INVALID FRAME RECEIVED ***************\n");
				LOG("* Expected %u, got %u\n",rxNextSeqNum,h->control.iframe.txseq);
			} else {
				// Check acks

				LOG("His sequence 0x%02x, acks 0x%02x\n",
					h->control.iframe.txseq,
					h->control.iframe.rxseq);


				if (linkFlags & LINK_FLAG_LINKUP) {
					rxNextSeqNum++;
					rxNextSeqNum&=0x7;

					linkFlags &= ~LINK_FLAG_PACKETSENT;

					handleInformationFrame(h);
					LOG("Sending packet for processing\n");
					Implementation::processPacket(getBuffer()+2,pBufPtr-4);

					if (!(linkFlags & LINK_FLAG_PACKETSENT)) {
						ackLastFrame();
					}
				} else {
					sendSupervisoryFrame(REJ);
					LOG("Link down, dropping frame\n");
				}
			}

		} else if (h->control.frame_type.flag & 2) {
			LOG("Unnumbered frame\n");
			handle_unnumbered();
			/* Unnumbered */
		} else {
			LOG("Supervisory frame\n");
			handle_supervisory();
			/* Supervisory */
		}


		pBufPtr=0;
	}

	static void processData(uint8_t bIn)
	{
		LOG("Process data: %d (0x%02x)\n",bIn,bIn);
		if (bIn==HDLC_escapeFlag) {
			unEscaping=true;
			return;
		}

		// Check unescape error ?
		if (bIn==HDLC_frameFlag && !unEscaping) {
			if (inPacket) {
				/* End of packet */
				if (pBufPtr) {
					LL_handleEvent<END_FRAME>();
					preProcessPacket();
					inPacket = false;
				}
			} else {
				/* Beginning of packet */
				pBufPtr = 0;
				inPacket = true;
				LL_handleEvent<START_FRAME>();
				crcgen.reset();
			}
		} else {
			if (unEscaping) {
				bIn^=HDLC_escapeXOR;
				unEscaping=false;
			}

			if (pBufPtr<Config::maxPacketSize) {
				getBuffer()[pBufPtr++]=bIn;
				// Update CRC
				/*crcSaveB = crcSaveA;
				crcSaveA = crcgen.get();*/
				crcgen.update(bIn);

			} else {
				// Process overrun error
			}
		}
	}
};

#ifndef SERPRO_EMBEDDED
#define HDLC_QUEUES \
	template<> PacketQueue SerPro::MyProtocol::txQueue=PacketQueue(); \
	template<> PacketQueue SerPro::MyProtocol::rxQueue=PacketQueue(); \
	template<> std::queue<Packet*> SerPro::MyProtocol::inputQueue = std::queue<Packet*>();\
	template<> bool SerPro::MyProtocol::linkReady = true;
#else
#define HDLC_QUEUES
#endif

#define IMPLEMENT_PROTOCOL_SerProHDLC(SerPro) \
	template<> SerPro::MyProtocol::buffer_size_t SerPro::MyProtocol::pBufPtr=0; \
	template<> SerPro::MyProtocol::cpuword_t SerPro::MyProtocol::txSeqNum=0; \
	template<> SerPro::MyProtocol::cpuword_t SerPro::MyProtocol::rxNextSeqNum=0; \
	template<> SerPro::MyProtocol::cpuword_t SerPro::MyProtocol::linkFlags=0; \
	template<> SerPro::MyProtocol::cpuword_t SerPro::MyProtocol::allowedPacketsInTransit=1; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::pSize=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::lastPacketSize=0; \
	template<> SerPro::MyProtocol::CRCTYPE SerPro::MyProtocol::crcgen=CRCTYPE(); \
	template<> SerPro::MyProtocol::cpuword_t SerPro::MyProtocol::unEscaping = 0; \
	template<> SerPro::MyProtocol::cpuword_t SerPro::MyProtocol::inPacket = 0; \
	template<> SerPro::MyProtocol::crc_t SerPro::MyProtocol::crcSaveA = 0; \
	template<> SerPro::MyProtocol::crc_t SerPro::MyProtocol::crcSaveB = 0; \
	template<> unsigned char SerPro::MyProtocol::_pBuf_internal[]={}; \
	template<> SerPro::MyProtocol::timer_t SerPro::MyProtocol::linktimer=timer_t(); \
	template<> SerPro::MyProtocol::timer_t SerPro::MyProtocol::retransmittimer=timer_t(); \
	template<> void SerPro::MyProtocol::sendByte(uint8_t byte) \
	{\
		if (byte==HDLC_frameFlag || byte==HDLC_escapeFlag) {\
	        SerPro::MySerial::write(HDLC_escapeFlag);\
			SerPro::MySerial::write(byte ^ HDLC_escapeXOR);\
		} else\
			SerPro::MySerial::write(byte);\
	}\
	template<> void SerPro::MyProtocol::sendPreamble() \
	{ \
	SerPro::MySerial::write( HDLC_frameFlag );\
	SerPro::MyProtocol::sendByte( (uint8_t)SerPro::MyConfig::HDLC::stationId ); \
	SerPro::MyProtocol::crcgen.update( (uint8_t)SerPro::MyConfig::HDLC::stationId ); \
	SerPro::MyProtocol::sendInformationControlField(); \
	}\
	template<> void SerPro::MyProtocol::sendSupervisoryFrame(supervisory_command c) \
	{ \
		uint8_t v = 0x01;          \
		v |= c<<2;                 \
		v |= (rxNextSeqNum<<5);    \
                                   \
		startPacket(0);            \
                                         \
 	    SerPro::MySerial::write( HDLC_frameFlag ); \
	sendByte( (uint8_t)SerPro::MyConfig::HDLC::stationId );   \
    	crcgen.update( (uint8_t)SerPro::MyConfig::HDLC::stationId );\
		sendByte(v);                                      \
		crcgen.update(v);                                 \
		sendSUPostamble();                                \
	} \
	template<> void SerPro::MyProtocol::sendPostamble()     \
	{                                                       \
		CRC16_ccitt::crc_t crc = crcgen.get();              \
		sendByte(crc>>8);                                   \
		sendByte(crc & 0xff);                               \
	SerPro::MySerial::write(HDLC_frameFlag);                      \
		SerPro::MySerial::flush();                                    \
		LL_handleEvent<END_XMIT>();                            \
		txSeqNum++;                                         \
		txSeqNum&=0x7;               \
                                                            \
		linkFlags |= LINK_FLAG_PACKETSENT;                  \
	}                                                       \
    HDLC_QUEUES
#endif
