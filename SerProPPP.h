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

#include "SerProHDLC.h"

#ifdef AVR
#include <avr/pgmspace.h>
#ifndef LOG
#define LOG(x...)
#endif
#else
#define PROGMEM
#ifndef LOG
#define LOG(x...) fprintf(stderr,x);
#endif
#endif

/* Some helpers */

/* These are only for 16-bit entries */
#define HI8(x) (((x)>>8)&0xff)
#define LO8(x) ((x)&0xff)

/* Protocols */

#define PROTOCOL_LCP   0xc021
#define PROTOCOL_IPCP  0x8021
#define PROTOCOL_IP    0x8021

#define LCP_CONFIGURE_REQUEST 1
#define LCP_CONFIGURE_ACK     2
#define LCP_CONFIGURE_NAK     3
#define LCP_CONFIGURE_REJECT  4
#define LCP_TERMINATE_REQUEST 5
#define LCP_TERMINATE_ACK     6
#define LCP_CODE_REJECT       7
#define LCP_PROTOCOL_REJECT   8
#define LCP_ECHO_REQUEST      9
#define LCP_ECHO_REPLY        10
#define LCP_DISCARD_REQUEST   11

/* State flags */
#define LCP_FLAG_PEER_ACKED 1
#define LCP_FLAG_WE_ACKED   2
#define LCP_FLAG_UP         4
#define IPCP_FLAG_PEER_ACKED 8
#define IPCP_FLAG_WE_ACKED   16
#define IPCP_FLAG_UP         32


#define LCP_OPTION_ASYNCMAP 2
#define LCP_OPTION_MAGIC    5
#define LCP_OPTION_MRU      1

#define IPCP_OPTION_IPADDRESS 3

/* HDLC-Specific */

#define HDLC_UNNUMBERED_INFORMATION 0x03

/*
 Our main class definition.
 TODO: document
 */
template<class Config, class Serial,class Timer=NoTimer>
struct PPPprotocolImplementation
{
	typedef SerProHDLC<Config,Serial,PPPprotocolImplementation,Timer> MyProtocol;
	/* Forwarded types */
	typedef typename MyProtocol::command_t command_t;
	typedef typename MyProtocol::buffer_size_t buffer_size_t;
	typedef typename MyProtocol::RawBuffer RawBuffer;

	static uint8_t txseq;
	static const unsigned char myMagic[4];
	static unsigned char ipAddress[4];

	static uint8_t flags;

	static int handleLCPOption(uint8_t option, const unsigned char *buf,
							   buffer_size_t size, int apply)
	{
		switch (option) {
		case LCP_OPTION_ASYNCMAP:
			return 0;
		case LCP_OPTION_MAGIC:
			return 0;
		default:
			LOG("Unknown LCP option 0x%02x\n",option);
			return -1;
		}
	}

	static int handleIPCPOption(uint8_t option, const unsigned char *buf,
							   buffer_size_t size, int apply)
	{
		switch (option) {
		case IPCP_OPTION_IPADDRESS:
			return 0;
		default:
			LOG("Unknown IPCP option 0x%02x\n",option);
			return -1;
		}
	}

	static int handleIPCPNAKOption(uint8_t option, const unsigned char *buf,
							   buffer_size_t size, int apply)
	{
		switch (option) {
		case IPCP_OPTION_IPADDRESS: // IP Address
			/* Peer wants to set our IP address */
			LOG("NAK SIZE %u\n",size);
			if (size==4) {
				memcpy(ipAddress,buf,size);
				sendIPCPRequest();
			}
			return 0;
            break;
		default:
			LOG("Unknown IPCP option 0x%02x\n",option);
			return -1;
		}
	}

	static void sendLCPReplyHeader()
	{
		MyProtocol::setEscapeLow(true);
		MyProtocol::startPacket(0);
		MyProtocol::sendPreamble(HDLC_UNNUMBERED_INFORMATION);
		MyProtocol::sendData(HI8(PROTOCOL_LCP));
		MyProtocol::sendData(LO8(PROTOCOL_LCP));
	}

	static void sendIPData(const uint8_t *buf, size_t size)
	{
		MyProtocol::startPacket(0);
		MyProtocol::sendPreamble(HDLC_UNNUMBERED_INFORMATION);
		MyProtocol::sendData(HI8(PROTOCOL_IP));
		MyProtocol::sendData(LO8(PROTOCOL_IP));

		MyProtocol::sendData(buf,size);

		MyProtocol::sendPostamble();
	}

	static void sendIPCPReplyHeader()
	{
		//MyProtocol::setEscapeLow(true);
		MyProtocol::startPacket(0);
		MyProtocol::sendPreamble(HDLC_UNNUMBERED_INFORMATION);
		MyProtocol::sendData(HI8(PROTOCOL_IPCP));
		MyProtocol::sendData(LO8(PROTOCOL_IPCP));
	}

	static void sendConfigRequest()
	{
		sendLCPReplyHeader();
		MyProtocol::sendData(LCP_CONFIGURE_REQUEST); // Config request
        MyProtocol::sendData(txseq++); // Seq.

		MyProtocol::sendData(0x0);
		MyProtocol::sendData(0x4 + 6 + 4); // 4 bytes.

		MyProtocol::sendData(LCP_OPTION_MAGIC); // Magic id
		MyProtocol::sendData(0x6); // Magic size

		MyProtocol::sendData((unsigned char*)myMagic,sizeof(myMagic)); // Magic

		MyProtocol::sendData(LCP_OPTION_MRU); // MRU
		MyProtocol::sendData(0x4); // MRU size

		MyProtocol::sendData(((Config::maxPacketSize-6)>>8)&0xff ); // MRU size
		MyProtocol::sendData(((Config::maxPacketSize-6)>>0)&0xff ); // MRU size

		MyProtocol::sendPostamble();
	}

	static void sendIPCPRequest()
	{
		sendIPCPReplyHeader();
		MyProtocol::sendData(0x1); // Config request
		MyProtocol::sendData(txseq++); // Seq.

		MyProtocol::sendData(0x0);
		MyProtocol::sendData(0x4 + 6); // 4 bytes.

		MyProtocol::sendData(IPCP_OPTION_IPADDRESS); // IP-Address
		MyProtocol::sendData(0x6); // Size
		MyProtocol::sendData(ipAddress,sizeof(ipAddress)); // Size
		MyProtocol::sendPostamble();
	}

	static void sendEchoReply(uint8_t seq)
	{
		sendLCPReplyHeader();
		MyProtocol::sendData(LCP_ECHO_REPLY); // Echo reply
		MyProtocol::sendData(seq); // Seq.

		MyProtocol::sendData(0);
        MyProtocol::sendData(8);

		MyProtocol::sendData((unsigned char*)myMagic,sizeof(myMagic)); // Magic
        MyProtocol::sendPostamble();
	}

	typedef int (*optionsHandler)(uint8_t option, const unsigned char *buf,buffer_size_t size, int apply);

	static void processLCPConfigRequest(uint8_t seq, const unsigned char *buf,
										buffer_size_t size)
	{
		if (processConfigRequests(PROTOCOL_LCP,seq,buf,size,&handleLCPOption))
			flags |= LCP_FLAG_WE_ACKED;
		else
			flags &= ~(LCP_FLAG_WE_ACKED|IPCP_FLAG_UP);

		checkLCP();
	}

	static void processLCPTerminateRequest(uint8_t seq, const unsigned char *buf, buffer_size_t size)
	{
		sendLCPReplyHeader();
		MyProtocol::sendData(LCP_TERMINATE_ACK);
		MyProtocol::sendData(seq);

		MyProtocol::sendData(0);
		MyProtocol::sendData(4); // No data

		MyProtocol::sendPostamble();
		flags = 0;
	}


	static void processIPCPConfigRequest(uint8_t seq, const unsigned char *buf,
										 buffer_size_t size) {
		if (processConfigRequests(PROTOCOL_IPCP,seq,buf,size,&handleIPCPOption)) {
			flags |= IPCP_FLAG_WE_ACKED;
		} else {
			flags &= ~(IPCP_FLAG_WE_ACKED|IPCP_FLAG_UP);
		}
        checkIPCP();
	}

	static void processIPCPConfigNAK(uint8_t seq, const unsigned char *buf,
									 buffer_size_t size) {
		processConfigRequests(PROTOCOL_IPCP,seq,buf,size,&handleIPCPNAKOption);
	}

	static void checkLCP() {
		if (flags & LCP_FLAG_UP)
			return; // Already UP

		if (flags & (LCP_FLAG_WE_ACKED|LCP_FLAG_PEER_ACKED)) {
			flags |= LCP_FLAG_UP;

            // No more need to escape
			MyProtocol::setEscapeLow(true);
			sendIPCPRequest();
		}
	}

	static void checkIPCP() {
		if (flags & IPCP_FLAG_UP)
			return; // Already UP

		/* TODO: notify upper layer link is up */
		if (flags & (IPCP_FLAG_WE_ACKED|IPCP_FLAG_PEER_ACKED)) {
			flags |= IPCP_FLAG_UP;
		}
	}

	static uint8_t processConfigRequests(unsigned short protocol, uint8_t seq, const unsigned char *buf,buffer_size_t size, optionsHandler handler)
	{
		int spos=0;
		int ack=1;
		uint16_t acksize=0,naksize=0;

		while (spos<size) {

			LOG("ConfigRequest type 0x%02x, len %02x\n",buf[spos],buf[spos+1]);
			int i;
			for (i=0;i<buf[spos+1]-2;i++) {
				LOGN( "0x%02x ",buf[spos+2+i]);
			}
			LOGN("\n");

			if (handler(buf[spos], &buf[spos+2], buf[spos+1]-2,0)<0) {
				ack=0;
				naksize+=buf[spos+1];
			} else {
				acksize+=buf[spos+1];
			}
			spos+=buf[spos+1];
		}

		MyProtocol::setEscapeLow(true);

		MyProtocol::startPacket(0);
		MyProtocol::sendPreamble(HDLC_UNNUMBERED_INFORMATION);

		MyProtocol::sendData(HI8(protocol));
		MyProtocol::sendData(LO8(protocol));

		LOG("ACK is %d\n",ack);
		if (ack) {
			MyProtocol::sendData(LCP_CONFIGURE_ACK);
		} else {
			MyProtocol::sendData(LCP_CONFIGURE_NAK); /* Reject ???? */
		}

		MyProtocol::sendData(seq);

		if (!ack) {
			acksize=naksize;
		}
		acksize += 4;

		MyProtocol::sendData(HI8(acksize));
		MyProtocol::sendData(LO8(acksize));

		spos=0;
		while (spos<size) {
			int i;
			for (i=0;i<buf[spos+1];i++) {
				LOGN( "0x%02x ",buf[spos+i]);
			}
			LOGN("\n");
			if (handler(buf[spos], &buf[spos+2], buf[spos+1]-2, ack)>=0) {
				if (ack)
					MyProtocol::sendData(&buf[spos], buf[spos+1]);
			} else {
				LOG("REJECTING option %d\n", buf[spos]);
				MyProtocol::sendData(&buf[spos], buf[spos+1]);
			}
			spos+=buf[spos+1];
		}

		MyProtocol::sendPostamble();

		return ack;
	}

	static buffer_size_t processLCP(const unsigned char *buf,
									buffer_size_t size)
	{
		uint16_t lcpsz = buf[3];
		lcpsz += ((uint16_t)buf[2]<<8);

		LOG("LCP code 0x%02x, id 0x%02x, len %d\n",buf[0],buf[1],lcpsz);

		switch(buf[0]) {

		case LCP_CONFIGURE_REQUEST: // ConfigRequest
			processLCPConfigRequest(buf[1], &buf[4],lcpsz-4);
			sendConfigRequest();
			break;

		case LCP_CONFIGURE_ACK: // Config ack
			flags |= LCP_FLAG_PEER_ACKED;
			checkLCP();
			break;

		case LCP_ECHO_REQUEST: // Echo-request
			/* Send back echo reply */
			LOG("Got echo request seq %u\n",buf[1]);
			sendEchoReply(buf[1]);
			break;

		case LCP_TERMINATE_REQUEST:
			processLCPTerminateRequest(buf[1],&buf[4],lcpsz);

			break;

		default:
            break;
		}
        return size - lcpsz;
	}


	static buffer_size_t processIPCP(const unsigned char *buf,
							buffer_size_t size)
	{
		uint16_t ipcpsz = buf[3];
		ipcpsz += ((uint16_t)buf[2]<<8);

		LOG("IPCP code 0x%02x, id %d, len %d\n",buf[0],buf[1],ipcpsz);

		switch(buf[0]) {

		case 1: // ConfigRequest
			processIPCPConfigRequest(buf[1], &buf[4],ipcpsz-4);
			break;

		case 2: // ConfigAck
			flags |= IPCP_FLAG_PEER_ACKED;
            checkIPCP();
			break;

		case 3: // ConfigNAK
			LOG("Got NAK\n");
			processIPCPConfigNAK(buf[1], &buf[4],ipcpsz-4);
			flags &= ~(IPCP_FLAG_PEER_ACKED|IPCP_FLAG_UP);
			checkIPCP();

		default:
            break;
		}

		return size - ipcpsz;
	}

	static void processIP(const unsigned char *buf, buffer_size_t size)
	{
		Config::IPHandler::processIP(buf,size);
	}

	static inline void processPacket(const unsigned char *buf, buffer_size_t size)
	{
		uint16_t proto;
		buffer_size_t sz = 0; // TODO - put packet size here.
		//callFunction(buf[0], buf+1, sz-1);

		proto = buf[1];
		proto += ((uint16_t)buf[0]<<8);

		LOG("Proto %04x\n",proto);

		if (proto == PROTOCOL_LCP) {
			processLCP( &buf[2],size-2);
			return;
		}
		if (!flags & LCP_FLAG_UP) {
			return; // NO LCP open, do not allow other protocols
		}

		if (proto == PROTOCOL_IPCP) {
			
			processIPCP( &buf[2],size-2);

		} else if (proto == PROTOCOL_IP) {

			processIP(&buf[2],size-2);

		} else {

			/* Send protocol reject */
            sendLCPReplyHeader();
			MyProtocol::sendData(LCP_PROTOCOL_REJECT); /* Protocol-reject */
            MyProtocol::sendData(txseq++); /* Seq */
			unsigned short sz =  4+ size;

			MyProtocol::sendData(HI8(sz));
            MyProtocol::sendData(LO8(sz));
			
			buffer_size_t p;

			LOG("Packet size is %d\n",size);

			for (p=0; p<size; p++)
				MyProtocol::sendData(buf[p]);
			MyProtocol::sendPostamble();

		}
	}

	static inline void processData(uint8_t bIn)
	{
		MyProtocol::processData(bIn);
	}
};


#define DECLARE_PPP_SERPRO(config,serial,name) \
	typedef PPPprotocolImplementation<config,serial> name; \
	template<> uint8_t name::txseq = 1; \
	template<> const unsigned char name::myMagic[4] =  {'A','R','D','U'}; \
	template<> unsigned char name::ipAddress[4] =  {0,0,0,0}; \
	template<> uint8_t name::flags = 0;

#define IMPLEMENT_SERPRO(name) \
	IMPLEMENT_PROTOCOL_SerProHDLC(name)


#endif
