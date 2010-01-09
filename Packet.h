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

#ifndef __PACKET_H__
#define __PACKET_H__

#include <inttypes.h>

class Packet
{
public:
	virtual void append(uint8_t b) = 0;
	virtual void append(uint16_t b) = 0;
	virtual void append(uint32_t b) = 0;
	virtual void append(uint8_t *buf, size_t size) = 0;
	virtual ~Packet() {}
};

#endif
