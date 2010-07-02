/*
 Based on information from
 http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
 */

#ifndef __CRC16_H__
#define __CRC16_H__

#include <inttypes.h>

struct CRC16_ccitt
{
	typedef uint16_t crc_t;
	crc_t crc;

	inline void reset()
	{
		crc = 0xffff;
	}

	void update(uint8_t data);

	inline crc_t get() {
		return crc;
	}
};

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
