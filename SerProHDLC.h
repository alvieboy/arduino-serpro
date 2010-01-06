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


#ifndef AVR
#include <stdio.h>
#define LOG(m...) fprintf(stderr,"[%d] ",getpid()); fprintf(stderr,m);
#else
#define LOG(m...)
#endif

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

template<class Config,class Serial,class Implementation> class SerProHDLC
{
public:
	static uint8_t const frameFlag = 0x7E;
	static uint8_t const escapeFlag = 0x7D;
	static uint8_t const escapeXOR = 0x20;

	/* Buffer */
	static unsigned char pBuf[Config::maxPacketSize];

	typedef CRC16_ccitt CRCTYPE;
	typedef CRCTYPE::crc_t crc_t;

	static CRCTYPE incrc,outcrc;

	typedef uint8_t command_t;


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
		r.buffer = pBuf+3;
		r.size = lastPacketSize;
		LOG("getRawBuffer() : size %u %u\n", r.size,lastPacketSize);
		return r;
	}

	static inline void sendByte(uint8_t byte)
	{
		if (byte==frameFlag || byte==escapeFlag || (forceEscapingLow&&byte<0x20)) {
			Serial::write(escapeFlag);
			Serial::write(byte ^ escapeXOR);
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
		// Not used 00-011
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
		pBufPtr=0;
	}

	static void sendPreamble()
	{
		Serial::write( frameFlag );
		sendByte( (uint8_t)Config::stationId );
		outcrc.update( (uint8_t)Config::stationId );
		sendInformationControlField();
	}

	static void sendPostamble()
	{
		CRC16_ccitt::crc_t crc = outcrc.get();
		sendByte(crc & 0xff);
		sendByte(crc>>8);
		Serial::write(frameFlag);
		Serial::flush();

		txSeqNum++;
		txSeqNum&=0x7; // Cap at 3-bits only.

		linkFlags |= LINK_FLAG_PACKETSENT;
	}
	static void sendSUPostamble()
	{
		CRC16_ccitt::crc_t crc = outcrc.get();
		sendByte(crc & 0xff);
		sendByte(crc>>8);
		Serial::write(frameFlag);
		Serial::flush();
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


	static void handle_supervisory()
	{
		LOG("Got supervisory frame\n");
		HDLC_header *h = (HDLC_header*)pBuf;
		supervisory_command c = (supervisory_command)(h->control.sframe.function);
		LOG("Got supervisory frame 0x%02x\n", c);
		switch (c) {
		case RR:
			LOG("RR, ack'ed 0x%02x\n", h->control.sframe.seq);
			break;
		default:
			LOG("Unhandled supervisory frame\n");
		}
	}

	static void handle_unnumbered()
	{
		HDLC_header *h = (HDLC_header*)pBuf;
		unnumbered_command c = (unnumbered_command)(h->control.value & 0xEC);
		LOG("Unnumbered frame 0x%02x (0x%02x)\n",c,h->control.value);
		switch(c) {
		case SNRM:
			sendUnnumberedFrame(UA);
			linkFlags |= LINK_FLAG_LINKUP;
			// Reset tx/rx sequences
			txSeqNum=0;
			rxNextSeqNum=0;
			LOG("Link up, NRM\n");
			break;
		case DM:
			linkFlags &= ~LINK_FLAG_LINKUP;
			LOG("Link down\n");
			break;
		case UA:
			linkFlags |= LINK_FLAG_LINKUP;
			// Reset tx/rx sequences
			txSeqNum=0;
			rxNextSeqNum=0;
			LOG("Link up, by our request\n");
			break;

		default:
			sendUnnumberedFrame(DM);
			linkFlags &= ~LINK_FLAG_LINKUP;
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
		Serial::write( frameFlag );
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

		Serial::write( frameFlag );
		sendByte( (uint8_t)Config::stationId );
		outcrc.update( (uint8_t)Config::stationId );
		sendByte(v);
		outcrc.update(v);
		sendSUPostamble();
	}

	static void ackLastFrame()
	{
		uint8_t v = RR << 2;
		v|= 0x01;
		v|= (rxNextSeqNum & 0x7)<<5;

		LOG("Acknowledging last frame 0x%02x\n",v);

		startPacket(0);
		Serial::write( frameFlag );
		sendByte( (uint8_t)Config::stationId );
		outcrc.update( (uint8_t)Config::stationId );
		sendByte(v);
		outcrc.update(v);
		sendSUPostamble();
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
		if (bIn==escapeFlag) {
			unEscaping=true;
			return;
		}

		// Check unescape error ?
		if (bIn==frameFlag && !unEscaping) {
			if (inPacket) {
				/* End of packet */
				if (pBufPtr) {
					preProcessPacket();
					inPacket = false;
				}
			} else {
				/* Beginning of packet */
				pBufPtr = 0;
				inPacket = true;
				incrc.reset();
			}
		} else {
			if (unEscaping) {
				bIn^=escapeXOR;
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
	template<> unsigned char SerPro::MyProtocol::pBuf[]={0};

#endif
