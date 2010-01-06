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

package com.alvie.arduino.serpro;

public class CRC16CCITT {

    int crc;

    public void reset()
    {
        crc = 0xffff;
    }

    public int get() {
        return crc;
    }

    public void update(byte d)
    {
        int data = d&0xff;
        data ^= crc&0xff;
        data ^= (data << 4)&0xff;
        crc = (((data << 8) | ((crc>>8)&0xff)) ^ (data >> 4)
               ^ (data << 3));
    }
};
