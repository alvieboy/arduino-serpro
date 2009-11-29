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

#include <inttypes.h>
#include "crc16.h"

#ifndef AVR
#include <stdio.h>
#define LOG(m,x...) printf(m,x);
#else
#define LOG(m...)
#endif

template<unsigned int MAX_PACKET_SIZE,
	class Serial,
	class Implementation>
	class SerProHDLC
{
public:
	static uint8_t const frameFlag = 0x7E;
	static uint8_t const escapeFlag = 0x7D;
	static uint8_t const escapeXOR = 0x20;

	/* Buffer */
	static unsigned char pBuf[MAX_PACKET_SIZE];
	static CRC16_ccitt incrc,outcrc;

	typedef CRC16_ccitt::crc_t crc_t;

	typedef uint8_t command_t;
	typedef uint8_t buffer_size_t;
	typedef uint16_t packet_size_t;

	static buffer_size_t pBufPtr;
	static command_t command, outCommand;
	static packet_size_t pSize,lastPacketSize;
	static bool unEscaping;
	static bool inPacket;

	struct RawBuffer {
		unsigned char *buffer;
		uint8_t size;
	};

	static inline RawBuffer getRawBuffer()
	{
		RawBuffer r;
		r.buffer = pBuf+1;
		r.size = lastPacketSize;
		return r;
	}

	static inline void sendByte(uint8_t byte)
	{
		if (byte==frameFlag || byte==escapeFlag) {
			Serial::write(escapeFlag);
			Serial::write(byte ^ escapeXOR);
		} else
			Serial::write(byte);
	}

	static void startPacket(command_t const command,packet_size_t len)
	{
		outCommand=command;
		outcrc.reset();
		pBufPtr=0;
		//outSize = len;
	}
	static void sendPreamble()
	{
		Serial::write(frameFlag);
		sendByte(outCommand);
		outcrc.update(outCommand);
	}

	static void sendPostamble()
	{
		CRC16_ccitt::crc_t crc = outcrc.get();
		sendByte(crc & 0xff);
		sendByte(crc>>8);
		Serial::write(frameFlag);
	}

	static void sendData(const unsigned char * const buf, packet_size_t size)
	{
		packet_size_t i;
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

	static void sendPacket(command_t const command, unsigned char * const buf, packet_size_t const size)
	{
		startPacket(command,size);
		sendPreamble();
		sendData(buf,size);
		sendPostamble();
	}

	static void preProcessPacket()
	{
		/* Check CRC */
		if (pBufPtr<3) {
			/* Empty/erroneous packet */
			LOG("Short packet received, len %u\n",pBufPtr);
			return;
		}
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
		lastPacketSize = pBufPtr-3;
		LOG("Command is %u\n", pBuf[0]);
		Implementation::processPacket(pBuf[0],pBuf+1,pBufPtr-3);
	}

	static void processData(uint8_t bIn)
	{
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

			if (pBufPtr<MAX_PACKET_SIZE) {
				pBuf[pBufPtr++]=bIn;
			}
		}
	}
};

#define IMPLEMENT_PROTOCOL_SerProHDLC(SerPro) \
	template<> SerPro::MyProtocol::buffer_size_t SerPro::MyProtocol::pBufPtr=0; \
	template<> SerPro::MyProtocol::command_t SerPro::MyProtocol::command=0; \
	template<> SerPro::MyProtocol::command_t SerPro::MyProtocol::outCommand=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::pSize=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::lastPacketSize=0; \
	template<> CRC16_ccitt SerPro::MyProtocol::incrc=CRC16_ccitt(); \
	template<> CRC16_ccitt SerPro::MyProtocol::outcrc=CRC16_ccitt(); \
	template<> bool SerPro::MyProtocol::unEscaping = false; \
	template<> bool SerPro::MyProtocol::inPacket = false; \
	template<> unsigned char SerPro::MyProtocol::pBuf[]={0};
