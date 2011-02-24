#if defined(AVR) || defined(ZPU)
#define SERPROLIBRARY
#endif

#ifdef SERPROLIBRARY
#include <serpro/crc16.h>
#else
#include "crc16.h"
#endif
#include <inttypes.h>

#ifdef AVR
#include <util/crc16.h>
#endif

#ifndef ZPU
void CRC16_ccitt::update(uint8_t data)
{
	crcminustwo = crcminusone;
	crcminusone = crc;
#ifndef AVR
	data ^= crc&0xff;
	data ^= data << 4;
	crc = ((((uint16_t)data << 8) | ((crc>>8)&0xff)) ^ (uint8_t)(data >> 4)
		   ^ ((uint16_t)data << 3));
#else
	crc = _crc_ccitt_update(crc, data);
#endif
}
#endif


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

#if 0
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

#endif

// MOVE this to other file

#ifdef AVR
uint16_t bswap_16(uint16_t v)
{
	return (v>>8) | ((v & 0xff) << 8);

}
uint32_t bswap_32(uint32_t v)
{
	return ((v>>24)
			|((v&0x00ff0000)>>8)
			|((v&0x0000ff00)<<8)
			|((v&0x000000ff)<<24));
}
#endif


