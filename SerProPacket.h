/*
 SerPro - A serial protocol for arduino intercommunication
 Copyright (C) 2009-2010 Alvaro Lopes <alvieboy@alvie.com>

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

template<class Config,
	class Serial,
	class Implementation>
	class SerProPacket
{
public:

/* Serial processor state */
	enum state {
		SIZE,
		SIZE2,
		COMMAND,
		PAYLOAD,
		CKSUM
	};

	/* Buffer */
	static unsigned char pBuf[Config::maxPacketSize];

	typedef uint8_t checksum_t;
	typedef uint8_t command_t;
	//typedef typename best_storage_class<number_of_bytes<MAX_PACKET_SIZE>::bytes>::type buffer_size_t;
    typedef uint8_t buffer_size_t;
	typedef uint16_t packet_size_t;

	static buffer_size_t pBufPtr;
	static checksum_t cksum,outCksum;
	static command_t command,outCommand;
	static packet_size_t lastPacketSize,pSize,pOutSize;

	static enum state st;

	struct RawBuffer {
		unsigned char *buffer;
		uint8_t size;
	};

	static inline RawBuffer getRawBuffer()
	{
		RawBuffer r;
		r.buffer = pBuf;
		r.size = lastPacketSize;
		return r;
	}
	static inline void startPacket(command_t command, packet_size_t size)
	{
		outCksum = command;
		outCommand = command;
		pOutSize = size;
	}

	static void sendPreamble()
	{
		packet_size_t rsize = pOutSize+1;
		if (rsize>127) {
			rsize |= 0x8000; // Set MSBit on MSB
			outCksum^= (rsize>>8);
			Serial::write((rsize>>8)&0xff);
		}
		outCksum^= (rsize&0xff);
		Serial::write(rsize&0xff);
		Serial::write(outCommand);
	}

	static void sendData(const unsigned char *buf,packet_size_t size)
	{
		packet_size_t i;
		for (i=0;i<size;i++) {
			outCksum^=buf[i];
		}
		Serial::write(buf,size);
	}

	static inline void sendData(unsigned char c)
	{
		outCksum^=c;
		Serial::write(c);
	}

	static inline void sendPostamble()
	{
		Serial::write(outCksum);
	}

	static void sendPacket(command_t const command, unsigned char * const buf, packet_size_t const size)
	{
		startPacket(command,size);
		sendPreamble();
		sendData(buf,size);
		sendPostamble();
	}

	static void processData(uint8_t bIn)
	{
		cksum^=bIn;

		switch(st) {
		case SIZE:
			cksum = bIn;
			if (bIn==0) {
				break; // Reset procedure.
			}
			if (bIn & 0x80) {
				pSize =((packet_size_t)(bIn&0x7F)<<8);
				st = SIZE2;
			} else {
				pSize = bIn;
				if (bIn>Config::maxPacketSize)
					break;
				pBufPtr = 0;
				st = COMMAND;
			}
			break;

		case SIZE2:
			pSize += bIn;
			if (bIn>Config::maxPacketSize)
				break;
			pBufPtr = 0;
			st = COMMAND;
			break;

		case COMMAND:

			command = bIn;
			pSize--;
			lastPacketSize=pSize;
			if (pSize>0)
				st = PAYLOAD;
			else
				st = CKSUM;
			break;
		case PAYLOAD:

			pBuf[pBufPtr++] = bIn;
			pSize--;
			if (pSize==0) {
				st = CKSUM;
			}
			break;

		case CKSUM:
			if (cksum==0) {
				Implementation::processPacket(command,pBuf,pBufPtr);
			}
			st = SIZE;
		}
	}
};

#define IMPLEMENT_PROTOCOL_SerProPacket(SerPro) \
	template<> SerPro::MyProtocol::buffer_size_t SerPro::MyProtocol::pBufPtr=0; \
	template<> SerPro::MyProtocol::checksum_t SerPro::MyProtocol::cksum=0; \
	template<> SerPro::MyProtocol::checksum_t SerPro::MyProtocol::outCksum=0; \
	template<> SerPro::MyProtocol::command_t SerPro::MyProtocol::command=0; \
	template<> SerPro::MyProtocol::command_t SerPro::MyProtocol::outCommand=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::pSize=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::lastPacketSize=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::pOutSize=0; \
	template<> SerPro::MyProtocol::state SerPro::MyProtocol::st=SIZE; \
	template<> unsigned char SerPro::MyProtocol::pBuf[]={0};
