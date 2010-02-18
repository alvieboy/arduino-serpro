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

#ifndef __SERPROCOMMON__
#define __SERPROCOMMON__

typedef enum {
	Master,
	Slave
} SerProImplementationType;

class NoTimer
{
public:
	typedef int timer_t;
	static inline timer_t addTimer( int (*cb)(void*), int milisseconds, void *data=0);
	static inline timer_t cancelTimer(const timer_t &t);
	static inline bool defined(const timer_t &t);
};

#endif
