#include "crc16.h"

void CRC16_ccitt::update(uint8_t data)
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


void CRC16::update(uint8_t data)
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

void CRC16_rfc1549::update(uint8_t data)
{
	uint8_t i;
	crc ^= data;
	for (i = 0; i < 8; ++i)
	{
		if (crc & 1)
			crc = (crc >> 1) ^ 0x8408;
		else
			crc = (crc >> 1);
	}
}
