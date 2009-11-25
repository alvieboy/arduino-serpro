#include <inttypes.h>

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

	typedef uint8_t checksum_t;
	typedef uint8_t command_t;
	typedef uint8_t buffer_size_t;
	typedef uint16_t packet_size_t;

	static buffer_size_t pBufPtr;
	static checksum_t cksum, outCksum;
	static command_t command, outCommand;
	static packet_size_t pSize;
	static bool unEscaping;
	static bool inPacket;

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
		//outSize = len;
	}
	static void sendPreamble()
	{
		cksum = outCommand;
		Serial::write(frameFlag);
		sendByte(outCommand);
	}

	static void sendPostamble()
	{
		sendByte(cksum);
		Serial::write(frameFlag);
	}

	static void sendData(unsigned char * const buf, packet_size_t size)
	{
		packet_size_t i;
		for (i=0;i<size;i++) {
			cksum^=buf[i];
			sendByte(buf[i]);
		}
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
		Implementation::processPacket(command,pBuf,pBufPtr);
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
	template<> SerPro::MyProtocol::checksum_t SerPro::MyProtocol::cksum=0; \
	template<> SerPro::MyProtocol::command_t SerPro::MyProtocol::command=0; \
	template<> SerPro::MyProtocol::command_t SerPro::MyProtocol::outCommand=0; \
	template<> SerPro::MyProtocol::packet_size_t SerPro::MyProtocol::pSize=0; \
	template<> bool SerPro::MyProtocol::unEscaping = false; \
	template<> bool SerPro::MyProtocol::inPacket = false; \
	template<> unsigned char SerPro::MyProtocol::pBuf[]={0};
