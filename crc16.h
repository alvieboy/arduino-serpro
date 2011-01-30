/*
 Based on information from
 http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
 */

#ifndef __CRC16_H__
#define __CRC16_H__

#include <inttypes.h>
#ifdef ZPU
#include <register.h>

struct CRC16_ccitt
{
	typedef uint16_t crc_t;
	//crc_t crc;

	inline void reset()
	{
		CRC16POLY = 0x8408; // CRC16-CCITT
		CRC16ACC=0xFFFF;
	}

	inline void update(uint8_t data) {
		CRC16APP=data;
	}

	inline crc_t get() {
		return CRC16ACC;
	}

	inline crc_t getMinusTwo() {
		return CRC16AM2;
	}

};
#else
struct CRC16_ccitt
{
	typedef uint16_t crc_t;
	crc_t crc,crcminusone,crcminustwo;


	inline void reset()
	{
		crc = 0xffff;
	}

	void update(uint8_t data);

	inline crc_t get() {
		return crc;
	}

	inline crc_t getMinusTwo() {
		return crcminustwo;
	}
};
#endif

struct CRC16 {

	typedef uint16_t crc_t;

	crc_t crc;

	inline void reset() {
		crc = 0xffff;
	}

	void update(uint8_t data);

	inline crc_t get() {
		return crc;
	}
};

struct CRC16_rfc1549 {

	typedef uint16_t crc_t;

	crc_t crc;

	inline void reset() {
		crc = 0xffff;
	}

	void update(uint8_t data);

	inline crc_t get() {
		return ~crc;
	}
};

#endif
