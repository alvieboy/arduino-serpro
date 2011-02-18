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
#include "SerProCommon.h"

#ifndef SERPRO_EMBEDDED

#include <iostream>


class Packet
{
public:

	virtual void append(const uint8_t &b) = 0;
	virtual void append(const uint16_t &b) = 0;
	virtual void append(const uint32_t &b) = 0;

	virtual void append(const int8_t &b) = 0;
	virtual void append(const int16_t &b) = 0;
	virtual void append(const int32_t &b) = 0;

	void append(const VariableBuffer &buf) {
		append(buf.size);
		appendBuffer(buf.buffer, buf.size);
	}

	template<unsigned int N>
		void append(const FixedBuffer<N> &buf) {
			appendBuffer(buf.buffer, N);
		}

	template<class STRUCT>
		void append(const STRUCT *s) { appendBuffer((uint8_t*)s,sizeof(STRUCT));}

	virtual void appendBuffer(const uint8_t *buf, size_t size) = 0;
	
	template<typename A,typename B>
	void append(const A &a, const B &b) {
		append(a);
		append(b);
	}
	template<typename A,typename B,typename C>
	void append(const A &a, const B &b, const C &c) {
		append(a);
		append(b);
		append(c);
	}
	template<typename A,typename B,typename C,typename D>
	void append(const A &a, const B &b, const C &c, const D &d) {
		append(a);
		append(b);
		append(c);
		append(d);
	}
	template<typename A,typename B,typename C,typename D,typename E>
	void append(const A &a, const B &b, const C &c, const D &d, const E &e) {
		append(a);
		append(b);
		append(c);
		append(d);
		append(e);
	}
	template<typename A,typename B,typename C,typename D,typename E,typename F>
	void append(const A &a, const B &b, const C &c, const D &d, const E &e, const F &f) {
		append(a);
		append(b);
		append(c);
		append(d);
		append(e);
		append(f);
	}

	virtual ~Packet() {}
};

#endif


#endif
