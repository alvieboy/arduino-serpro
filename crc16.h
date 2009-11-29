#include <inttypes.h>

/*
 Based on information from
 http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
 */

#ifndef __CRC16_H__
#define __CRC16_H__

#ifdef AVR
#include <util/crc16.h>
#endif

struct CRC16_ccitt
{
	typedef uint16_t crc_t;
	crc_t crc;

	inline void reset()
	{
		crc = 0xffff;
	}

	void update(uint8_t data)
	{
#ifndef AVR
		data ^= crc&0xff;
		data ^= data << 4;
		crc = ((((uint16_t)data << 8) | ((crc>>8)&0xff)) ^ (uint8_t)(data >> 4)
			   ^ ((uint16_t)data << 3));
#else
		crc = _crc_ccitt_update(crc, data);
#endif
	}

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

	void update(uint8_t data)
	{
#ifndef AVR
		uint8_t i;
		crc ^= data;
		for (i = 0; i < 8; ++i)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xA001;
			else
				crc = (crc >> 1);
		}
#else
		crc = _crc16_update(crc,data);
#endif
	}

	inline crc_t get() {
		return crc;
	}
};

#endif
