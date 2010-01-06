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

public class SerPro implements SerProProtocol.SerProPacketListener {

    CommandListener [] listeners;

    public SerPro(SerProProtocol protocol, int numCommands) {
        proto=protocol;
        proto.addPacketListener(this);
        listeners = new CommandListener[numCommands];
    }

    private SerProProtocol proto;

    public interface CommandListener {
        public void handleCommand(SerPro serpro, SerPro.SerProBuffer buffer);
    };

    public class SerProBuffer
    {
        byte [] payload;
        int pos;
        SerProBuffer(byte [] data) {
            payload=data;
            pos=0;
        }
        public byte getU8() {
            return payload[pos++];
        }

        public int getU16() {
            byte l = payload[pos++];
            byte h = payload[pos++];
            return (int)l + ((int)h << 8);
        }
        public byte [] getAll() {
            return payload;
        }
    };

    public void setPort(SerialPort port, int baudrate)
    {
        proto.setPort(port,baudrate);
    }

    public synchronized void sendPacket(SerProPacket packet) {
        proto.queueTransmit(packet);
    }

    public SerProPacket createPacket(byte command)
    {
        SerProPacket p = proto.createPacket();
        p.append(command);
        return p;
    }

    public synchronized void sendPacket(byte command, byte[] payload) {

        SerProPacket p = proto.createPacket();
        p.append(command);
        p.append(payload);

        proto.queueTransmit(p);
    }

    public void sendPacket(byte command) {
        byte [] data = new byte[0];
        sendPacket(command,data);
    }

    public void sendPacket(byte command,byte value) {
        byte [] payload = new byte[1];
        payload[0]=value;
        sendPacket(command,payload);
    }

    public void handlePacket(byte command, byte [] packet)
    {
        //System.out.println("Command "+(command&0xff));
        if (listeners[command] != null) {
            listeners[command].handleCommand(this,new SerProBuffer(packet));
        } else {
            System.out.println("No listener for command "+(command&0xff));
        }
    }

    public void addCommandListener(byte command, CommandListener listener)
    {
        //System.out.println("Add listener for command "+(command&0xff));
        listeners[command&0xff] = listener;
    }

};
