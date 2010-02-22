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
#include "crc16.h"
#include "SerProCommon.h"
#include "Packet.h"
#ifndef AVR
#include <queue>
#else
namespace std {
	template<class T> class queue {
	public:
		size_t size() const;
		void pop();
		void push(const T&);
		T &front() const;
	};
};
#endif

#ifndef AVR
#include <stdio.h>
#define LOG(m...) /* fprintf(stderr,"[%d] ",getpid()); fprintf(stderr,m); */
#else
#define LOG(m...)
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

	void sendData(uint8_t *buf, size_t size)
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
		LOG("Packet add: %02x\n",b);
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
		sendByte(crc & 0xff);
		sendByte((crc>>8)&0xff);
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
		append((uint8_t)(value&0xff));
		append((uint8_t)((value>>8)&0xff));
	}

	void append(const uint32_t &value) {
		append((value &0xff));
		append((value>>8 &0xff));
		append((value>>16 &0xff));
		append((value>>24 &0xff));
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
		LOG("Packets to be acked: %d\n",(tx-ack)&0x7);
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
			LOG("PEEK packet: %p (index %u)",r,(tx-1)&0x7);
			return r;
		} else {
			return NULL;
		}
	}
	void ackUpTo(uint8_t num)
	{
		LOG("ACK'ing up to sequance %u\n",num);
		uint8_t i = ack;
		while (i!=num) {
			if (slots[i]) {
				/* Remove */
				LOG("Acking %u\n",i);
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
static inline void handleEvent() {
}

template<class Config,class Serial,class Implementation,class Timer> class SerProHDLC
{
public:
	/* Buffer */
	static unsigned char pBuf[Config::maxPacketSize];

	typedef CRC16_ccitt CRCTYPE;
	typedef CRCTYPE::crc_t crc_t;

	static CRCTYPE incrc,outcrc;

	typedef uint8_t command_t;

	typedef HDLCPacket<Config,Serial> MyPacket;
	static PacketQueue txQueue, rxQueue;
	static std::queue<Packet*> inputQueue;

	typedef typename best_storage_class< number_of_bytes<Config::maxPacketSize>::bytes >::type buffer_size_t;
	//typedef uint16_t buffer_size_t;
	typedef uint16_t packet_size_t;

	static buffer_size_t pBufPtr;
	static packet_size_t pSize,lastPacketSize;

	/* HDLC parameters extracted from frame */
	static uint8_t inAddressField;
	static uint8_t inControlField;

	/* HDLC control data */
	static uint8_t txSeqNum;        // Transmit sequence number
	static uint8_t rxNextSeqNum;    // Expected receive sequence number

	static bool unEscaping;
	static bool forceEscapingLow;
	static bool inPacket;

	struct RawBuffer {
		unsigned char *buffer;
		buffer_size_t size;
	};

	static uint8_t linkFlags;

#define LINK_FLAG_LINKUP 1
#define LINK_FLAG_PACKETSENT 2

	struct HDLC_header {
		uint8_t address;
		union {
			struct {
				uint8_t flag: 1;   /* Always zero */
				uint8_t txseq: 3;  /* TX sequence number */
				uint8_t poll: 1;   /* poll/final */
				uint8_t rxseq: 3;  /* RX sequence number */
			} iframe;

			struct {
				uint8_t flag: 2;   /* '1''0' for S-frame */
				uint8_t function: 2; /* Supervisory function */
				uint8_t poll: 1;     /* Poll/Final */
				uint8_t seq: 3;    /* Sequence number */
			} sframe;

			struct {
				uint8_t flag: 2;   /* '1''1' for U-Frame */
				uint8_t modifier: 2; /* Modifier bits */
				uint8_t poll: 1;   /* Poll/Final */
				uint8_t function: 3; /* Function */
			} uframe;

			struct {
				uint8_t flag: 2;  /* LSB-MSB:  0X: I-frame, 10: S-Frame, 11: U-Frame */
				uint8_t unused: 6;
			} frame_type;
			uint8_t value;
		} control;
	};


	static inline void setEscapeLow(bool a)
	{
		forceEscapingLow=a;
	}

	static inline void dumpPacket() { /* Debuggin only */
		unsigned i;
		LOG("Packet: %d bytes\n", lastPacketSize);
		LOG("Dump (hex): ");
		for (i=0; i<lastPacketSize; i++) {
			LOG("0x%02X ", (unsigned)pBuf[i+1]);
		}
		LOG("\n");
	}
	static inline RawBuffer getRawBuffer()
	{
		RawBuffer r;
		r.buffer = pBuf+2;
		r.size = lastPacketSize;
		LOG("getRawBuffer() : size %u %u\n", r.size,lastPacketSize);
		return r;
	}

	static void sendByte(uint8_t byte)
	{
		if (byte==HDLC_frameFlag || byte==HDLC_escapeFlag || (forceEscapingLow&&byte<0x20)) {
			Serial::write(HDLC_escapeFlag);
			Serial::write(byte ^ HDLC_escapeXOR);
		} else
			Serial::write(byte);
	}

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
		LOG("Link timeout, retrying\n");
		startLink();
		return 0;
	}

	static int retransmitTimerExpired(void*d)
	{
		if (isLinkUp()) {
			/* Retransmit queued frames */
			Packet *p = txQueue.peek();
			if (p)
				queueTransmit(p);
		}
		return 0;
	}

	static void startLink()
	{
		if (Timer::defined(linktimer)) {
			linktimer = Timer::cancelTimer(linktimer);
		}
		linktimer = Timer::addTimer( &linkExpired, 1000);
		sendUnnumberedFrame( SNURM );
	}

	static inline void sendInformationControlField()
	{
		uint8_t ifield;
		ifield = txSeqNum<<1 | rxNextSeqNum<<5 | 0x10;
		sendByte( ifield );
		outcrc.update( ifield );
	}

	static void startPacket(packet_size_t len)
	{
		outcrc.reset();
		handleEvent<START_XMIT>();
		pBufPtr=0;
	}

	static void startXmit()
	{
		if (Config::implementationType==Master) {
			HDLCPacket<Config,Serial> *p = static_cast<HDLCPacket<Config,Serial>*>(txQueue.peek());
			// Compute control field.
			uint8_t control;
			control = (txQueue.peekIndex())<<1;
			control |= 0x10; // Poll
			control |= (rxNextSeqNum<<5);
			p->send(control);
			if (Timer::defined(retransmittimer)) {
				retransmittimer = Timer::cancelTimer(retransmittimer);
			}
			retransmittimer = Timer::addTimer( &retransmitTimerExpired, 500);
		}
	}

	static void checkXmit()
	{
		if (Config::implementationType==Master) {
			LOG("Checking Xmit queue...\n");
			if (txQueue.toBeAcked()==0 && inputQueue.size()>0) {
				Packet *p = inputQueue.front();
				inputQueue.pop();
				LOG("Queuing packet %p\n",p);
				queueTransmit(p);
			}
		}
	}

	static int queueTransmit(Packet *p)
	{
		if (Config::implementationType==Master) {

			if (txQueue.toBeAcked()==0) {
				txQueue.queue( p );
				startXmit();
			} else {
				inputQueue.push(p);
			}
			return 1;
		}
		return 0;
	}

	static void sendPreamble()
	{
		Serial::write( HDLC_frameFlag );
		sendByte( (uint8_t)Config::stationId );
		outcrc.update( (uint8_t)Config::stationId );
		sendInformationControlField();
	}

	static void sendPostamble()
	{
		CRC16_ccitt::crc_t crc = outcrc.get();
		sendByte(crc & 0xff);
		sendByte(crc>>8);
		Serial::write(HDLC_frameFlag);
		Serial::flush();
		handleEvent<END_XMIT>();
		txSeqNum++;
		txSeqNum&=0x7; // Cap at 3-bits only.

		linkFlags |= LINK_FLAG_PACKETSENT;
	}
	static void sendSUPostamble()
	{
		CRC16_ccitt::crc_t crc = outcrc.get();
		sendByte(crc & 0xff);
		sendByte(crc>>8);
		Serial::write(HDLC_frameFlag);
		Serial::flush();
		handleEvent<END_XMIT>();
	}

	static void sendData(const unsigned char * const buf, packet_size_t size)
	{
		packet_size_t i;
		LOG("Sending %d payload\n",size);
		for (i=0;i<size;i++) {
			outcrc.update(buf[i]);
			sendByte(buf[i]);
		}
	}

	static void sendData(unsigned char c)
	{
		outcrc.update(c);
		sendByte(c);
	}

	static void sendCommandPacket(command_t const command, unsigned char * const buf, packet_size_t const size)
	{
		startPacket(size);
		sendPreamble();
		outcrc.update( command );
		sendData(command);
		sendData(buf,size);
		sendPostamble();
	}

	static inline void deferReply()
	{
		linkFlags |= LINK_FLAG_PACKETSENT;
	}

	static Packet *createPacket()
	{
		return new MyPacket();
	}

	static void handle_supervisory()
	{
		LOG("Got supervisory frame\n");
		HDLC_header *h = (HDLC_header*)pBuf;
		supervisory_command c = (supervisory_command)(h->control.sframe.function);
		LOG("Got supervisory frame 0x%02x\n", c);
		switch (c) {
		case RR:
			LOG("RR, ack'ed 0x%02x\n", h->control.sframe.seq);
			if (Config::implementationType==Master) {

				txQueue.ackUpTo(h->control.sframe.seq);

				if (Timer::defined(retransmittimer) && txQueue.toBeAcked()==0) {
					retransmittimer = Timer::cancelTimer(retransmittimer);
				}

				checkXmit();
			}
			break;
		case REJ:
			if (Config::implementationType==Master) {
				LOG("Got REJ for sequence %u\n",h->control.sframe.seq);
				break;
			}
		default:
			LOG("Unhandled supervisory frame\n");
		}
	}

	static inline bool isLinkUp() {
		return !!(linkFlags & LINK_FLAG_LINKUP);
	}

	static void setLinkUP()
	{
		linkFlags |= LINK_FLAG_LINKUP;
		handleEvent<LINK_UP>();
		checkXmit();
	}
	static void setLinkDOWN()
	{
		linkFlags &= ~LINK_FLAG_LINKUP;
		handleEvent<LINK_DOWN>();
	}

	static void handle_unnumbered()
	{
		HDLC_header *h = (HDLC_header*)pBuf;
		unnumbered_command c = (unnumbered_command)(h->control.value & 0xEC);
		LOG("Unnumbered frame 0x%02x (0x%02x)\n",c,h->control.value);
		switch(c) {
		case SNURM:
			sendUnnumberedFrame(UA);
			setLinkUP();
			// Reset tx/rx sequences
			txSeqNum=0;
			rxNextSeqNum=0;
			LOG("Link up, NRM\n");
			break;
		case DM:
			setLinkDOWN();
			LOG("Link down\n");
			break;
		case UA:
			setLinkUP();
			// Reset tx/rx sequences
			txSeqNum=0;
			rxNextSeqNum=0;
			LOG("Link up, by our request\n");
			if (Config::implementationType==Master) {
				if (Timer::defined(linktimer))
					linktimer = Timer::cancelTimer(linktimer);
			}
			break;

		default:
			sendUnnumberedFrame(DM);
			setLinkDOWN();
			LOG("Link down\n");
			break;
		}
	}

	static void sendUnnumberedFrame(unnumbered_command c)
	{
		uint8_t v = (uint8_t)c;
		v |= 0x03;

		startPacket(0);
		LOG("V: %02x c=%02x\n",v,c);
		Serial::write( HDLC_frameFlag );
		sendByte( (uint8_t)Config::stationId );
		outcrc.update( (uint8_t)Config::stationId );
		sendByte(v);
		outcrc.update(v);
		sendSUPostamble();
	}

	static void sendSupervisoryFrame(supervisory_command c)
	{
		uint8_t v = 0x01;
		v |= c<<2;
		v |= (rxNextSeqNum<<5);

		startPacket(0);

		Serial::write( HDLC_frameFlag );
		sendByte( (uint8_t)Config::stationId );
		outcrc.update( (uint8_t)Config::stationId );
		sendByte(v);
		outcrc.update(v);
		sendSUPostamble();
	}

	static void ackLastFrame()
	{
		if (Config::implementationType == Slave) {
			uint8_t v = RR << 2;
			v|= 0x01;
			v|= (rxNextSeqNum & 0x7)<<5;

			LOG("Acknowledging last frame 0x%02x\n",v);

			startPacket(0);
			Serial::write( HDLC_frameFlag );
			sendByte( (uint8_t)Config::stationId );
			outcrc.update( (uint8_t)Config::stationId );
			sendByte(v);
			outcrc.update(v);
			sendSUPostamble();
		}
	}

	static void handleInformationFrame(const HDLC_header *h)
	{
		if (Config::implementationType==Master) {
			/* Ack frames */
			txQueue.ackUpTo(h->control.iframe.rxseq);
			checkXmit();
		}
	}

	static void preProcessPacket()
	{
		HDLC_header *h = (HDLC_header*)pBuf;
		/* Check CRC */
		if (pBufPtr<4) {
			/* Empty/erroneous packet */
			LOG("Short packet received, len %u\n",pBufPtr);
			return;
		}

		/* Make sure packet is meant for us. We can safely check
		 this before actually computing CRC */

		packet_size_t i;
		incrc.reset();
		for (i=0;i<pBufPtr-2;i++) {
			incrc.update(pBuf[i]);
		}
		crc_t pcrc = *((crc_t*)&pBuf[pBufPtr-2]);
		if (pcrc!=incrc.get()) {
			/* CRC error */
			LOG("CRC ERROR, expected 0x%04x, got 0x%04x\n",incrc.get(),pcrc);
			handleEvent<CRC_ERROR>();
			return;
		}
		LOG("CRC MATCH 0x%04x, got 0x%04x\n",incrc.get(),pcrc);
		lastPacketSize = pBufPtr-4;
		LOG("Packet details: destination ID %u, control 0x%02x\n", h->address,h->control.value);

		if ((h->control.frame_type.flag & 1) == 0) {
			/* Information  */
			LOG("Information frame\n");

			/* Ensure this packet comes in sequence */

			if (rxNextSeqNum != h->control.iframe.txseq) {
				// Reject frame here ????
				sendSupervisoryFrame(REJ);

				LOG("************* INVALID FRAME RECEIVED ***************\n");
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

					Implementation::processPacket(pBuf+2,pBufPtr-4);

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
					handleEvent<END_FRAME>();
					preProcessPacket();
					inPacket = false;
				}
			} else {
				/* Beginning of packet */
				pBufPtr = 0;
				inPacket = true;
				handleEvent<START_FRAME>();
				incrc.reset();
			}
		} else {
			if (unEscaping) {
				bIn^=HDLC_escapeXOR;
				unEscaping=false;
			}

			if (pBufPtr<Config::maxPacketSize) {
				pBuf[pBufPtr++]=bIn;
			} else {
				// Process overrun error
			}
		}
	}
};

#define IMPLEMENT_PROTOCOL_SerProHDLC(SerPro) \
	template<> SerPro::MyProtocol::buffer_size_t SerPro::MyProtocol::pBufPtr=0; \
	template<> uint8_t SerPro::MyProtocol::txSeqNum=0; \
	template<> uint8_t SerPro::MyProtocol::rxNextSeqNum=0; \
	template<> uint8_t SerPro::MyProtocol::linkFlags=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::pSize=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::lastPacketSize=0; \
	template<> SerPro::MyProtocol::CRCTYPE SerPro::MyProtocol::incrc=CRCTYPE(); \
	template<> SerPro::MyProtocol::CRCTYPE SerPro::MyProtocol::outcrc=CRCTYPE(); \
	template<> bool SerPro::MyProtocol::unEscaping = false; \
	template<> bool SerPro::MyProtocol::forceEscapingLow = false; \
	template<> bool SerPro::MyProtocol::inPacket = false; \
	template<> unsigned char SerPro::MyProtocol::pBuf[]={0}; \
	template<> SerPro::MyProtocol::timer_t SerPro::MyProtocol::linktimer=timer_t(); \
	template<> SerPro::MyProtocol::timer_t SerPro::MyProtocol::retransmittimer=timer_t(); \
	template<> PacketQueue SerPro::MyProtocol::txQueue=PacketQueue(); \
	template<> PacketQueue SerPro::MyProtocol::rxQueue=PacketQueue(); \
	template<> std::queue<Packet*> SerPro::MyProtocol::inputQueue = std::queue<Packet*>();

#endif
