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

import java.io.*;
import java.util.*;
import gnu.io.*;
import com.alvie.arduino.serpro.*;

public interface SerProProtocol
{
    /*void sendData(byte [] values);
    void sendData(byte value);
    void processData(byte value);
    */
    SerProPacket createPacket();
    /*
     void startPacket(byte command, int payload_size);
    void sendPreamble();
    void sendPostamble();
    */
    void queueTransmit(SerProPacket packet);

    void addPacketListener(SerProPacketListener listener);

    void setPort(SerialPort port, int baudrate);

    public interface SerProPacketListener {
        void handlePacket(byte command, byte [] payload);
    };
};
