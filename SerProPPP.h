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

/*
 Our main class definition.
 TODO: document
 */
template<class Config, class Serial,class Timer=NoTimer>
struct PPPprotocolImplementation
{
	typedef SerProHDLC<Config,Serial,PPPprotocolImplementation,Timer,CRC16_rfc1549> MyProtocol;
	/* Forwarded types */
	typedef typename MyProtocol::command_t command_t;
	typedef typename MyProtocol::buffer_size_t buffer_size_t;
	typedef typename MyProtocol::RawBuffer RawBuffer;

	static uint8_t txseq;
    static const unsigned char myMagic[4];

    static int handleLCPOption(uint8_t option, const unsigned char *buf,
							   buffer_size_t size, int apply)
	{
		switch (option) {
		case 0x02: // Asyncmap
			return 0;
		case 0x05: // Magic
			return 0;
		default:
			LOG("Unknown LCP option 0x%02x\n",option);
			return -1;
		}
	}

	static void sendLCPReplyHeader()
	{
		MyProtocol::setEscapeLow(true);
		MyProtocol::startPacket(0);
		MyProtocol::sendPreamble(0x03);
		MyProtocol::sendData(0xc0);
		MyProtocol::sendData(0x21);
	}

	static void sendConfigRequest()
	{
		sendLCPReplyHeader();
		MyProtocol::sendData(0x1); // Config request
        MyProtocol::sendData(txseq++); // Seq.
		MyProtocol::sendData(0x0);
		MyProtocol::sendData(0x4 + 6); // 4 bytes.
		MyProtocol::sendData(0x5); // Magic id
        MyProtocol::sendData(0x6); // Magic size
		MyProtocol::sendData((unsigned char*)myMagic,sizeof(myMagic)); // Magic

		// TODO - ADD MRU here.

        MyProtocol::sendPostamble();
	}

	static void sendEchoReply(uint8_t seq)
	{
		sendLCPReplyHeader();
		MyProtocol::sendData(10); // Echo reply
		MyProtocol::sendData(seq); // Seq.
		MyProtocol::sendData(0);
        MyProtocol::sendData(8);
		MyProtocol::sendData((unsigned char*)myMagic,sizeof(myMagic)); // Magic
        MyProtocol::sendPostamble();
	}

	typedef int (*optionsHandler)(uint8_t option, const unsigned char *buf,buffer_size_t size, int apply);

	static void processConfigRequests(uint8_t seq, const unsigned char *buf,buffer_size_t size, optionsHandler handler)
	{
	}

	static void processLCPConfigRequest(uint8_t seq, const unsigned char *buf,
										buffer_size_t size)
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
			if (handleLCPOption(buf[spos], &buf[spos+2], buf[spos+1]-2,0)<0) {
				ack=0;
				naksize+=buf[spos+1];
			} else {
				acksize+=buf[spos+1];
			}
            spos+=buf[spos+1];
		}

		MyProtocol::setEscapeLow(true);

		MyProtocol::startPacket(0);
		MyProtocol::sendPreamble(0x03);

		MyProtocol::sendData(0xc0);
		MyProtocol::sendData(0x21);

		LOG("ACK is %d\n",ack);
		if (ack) {
			MyProtocol::sendData(0x2);
		} else {
			MyProtocol::sendData(0x3);
		}

		MyProtocol::sendData(seq);

		if (!ack)
			acksize=naksize;

		acksize += 4;

		MyProtocol::sendData((acksize>>8)&0xff);
		MyProtocol::sendData(acksize&0xff);

		spos=0;
		while (spos<size) {
			int i;
			for (i=0;i<buf[spos+1];i++) {
				LOGN( "0x%02x ",buf[spos+i]);
			}
			LOGN("\n");
			if (handleLCPOption(buf[spos], &buf[spos+2], buf[spos+1]-2, ack)>=0) {
				MyProtocol::sendData(&buf[spos], buf[spos+1]);
			} else {
				LOG("REJECTING option %d\n", buf[spos]);
			}
			spos+=buf[spos+1];
		}

		MyProtocol::sendPostamble();
		sendConfigRequest();
		MyProtocol::setEscapeLow(false);
	}

	static void processLCP(const unsigned char *buf,
						   buffer_size_t size)
	{
		uint16_t lcpsz = buf[3];
		lcpsz += ((uint16_t)buf[2]<<8);
		LOG("LCP code 0x%02x, id 0x%02x, len %d\n",buf[0],buf[1],lcpsz);
		switch(buf[0]) {
		case 1: // ConfigRequest
			processLCPConfigRequest(buf[1], &buf[4],lcpsz-4);
			break;
		case 9: // Echo-request
			/* Send back echo reply */
            LOG("Got echo request seq %u\n",buf[1]);
            sendEchoReply(buf[1]);
            break;
		default:
            break;
		}
	}
	static void processIPCPConfigRequest(uint8_t seq, const unsigned char *buf,
										buffer_size_t size)
	{
		int spos=0;
		while (spos<size) {
			LOG("ConfigRequest type 0x%02x, len %02x\n",buf[spos],buf[spos+1]);
			switch(buf[0]) {
			case 3: // IP-address
				/* We don't need its IP address. */
				break;
			default:
                break;
			}
			spos+=buf[spos+1];
		};
	}
	static void processIPCP(const unsigned char *buf,
						   buffer_size_t size)
	{
		uint16_t ipcpsz = buf[3];
		ipcpsz += ((uint16_t)buf[2]<<8);
		LOG("IPCP code 0x%02x, id %d, len %d\n",buf[0],buf[1],ipcpsz);
		switch(buf[0]) {
		case 1: // ConfigRequest
			processIPCPConfigRequest(buf[1], &buf[4],ipcpsz-4);
			break;
		default:
            break;
		}
	}

	static inline void processPacket(const unsigned char *buf,
									 buffer_size_t size)
	{
		uint16_t proto;
		buffer_size_t sz = 0; // TODO - put packet size here.
		//callFunction(buf[0], buf+1, sz-1);

		proto = buf[1];
		proto += ((uint16_t)buf[0]<<8);
		LOG("Proto %04x\n",proto);
		if (proto == 0xc021) {
			processLCP( &buf[2],size-2);
		} else if (proto == 0x8021) {
			processIPCP( &buf[2],size-2);
		} else {
			/* Send protocol reject */
            sendLCPReplyHeader();
			MyProtocol::sendData(0x8); /* Protocol-reject */
            MyProtocol::sendData(txseq++); /* Seq */
			unsigned short sz =  4+ size;
			MyProtocol::sendData(sz>>8);
            MyProtocol::sendData(sz&0xff);
			
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
	template<> const unsigned char name::myMagic[4] =  {'A','R','D','U'};


#define IMPLEMENT_SERPRO(name) \
	IMPLEMENT_PROTOCOL_SerProHDLC(name)


#endif
