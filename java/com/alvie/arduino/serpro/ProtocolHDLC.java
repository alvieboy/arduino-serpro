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

public class ProtocolHDLC implements SerialPortEventListener, SerProProtocol
{
    SerProPacketListener packetListener;
    private SerialPort port;
    InputStream input;
    OutputStream output;
    static final int MAX_PACKET_SIZE = 4096;
    boolean linkUp;

    byte outCommand;
    CRC16CCITT incrc,outcrc;
    byte [] pBuf;
    int pBufPtr;
    int lastPacketSize;
    boolean inPacket;
    boolean unEscaping;

    int txSeqNum;
    int rxSeqNum;

    Timer retransmit_timer,link_timer;

    static final byte frameFlag = 0x7E;
    static final byte escapeFlag = 0x7D;
    static final byte escapeXOR = 0x20;
    boolean noXmitCheck;

    HDLCPacketQueue txQueue;
    HDLCPacketQueue ackQueue;

    static class Lock extends Object {
    };

    Lock txLock;

    public ProtocolHDLC()
    {
        incrc = new CRC16CCITT();
        outcrc = new CRC16CCITT();
        pBuf = new byte[MAX_PACKET_SIZE];
        txQueue = new HDLCPacketQueue();
        ackQueue= new HDLCPacketQueue();
        linkUp = false;
        txLock = new Lock();
    }

    public void setPort(SerialPort iport, int baudrate) {
        if (port!=null)
            port.close();
        port = iport;
        try {
            if (null!=link_timer)
                link_timer.cancel();
            if (null!=retransmit_timer)
                retransmit_timer.cancel();
            input = port.getInputStream();
            output = port.getOutputStream();
            port.setSerialPortParams(baudrate, SerialPort.DATABITS_8, SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
            port.setRTS(false);
            port.setFlowControlMode(SerialPort.FLOWCONTROL_NONE);
            port.addEventListener(this);
            port.notifyOnDataAvailable(true);
            startLink();
        } catch (java.io.IOException x) {
            System.out.println("IO Exception");
        } catch (gnu.io.UnsupportedCommOperationException x) {
            System.out.println("UnsupportedCommOperation Exception");
        } catch (java.util.TooManyListenersException x) {
            System.out.println("TooManyListeners Exception");
        }
    };


    void dumpReceivedPacket(byte [] values, int size)
    {
        System.out.print("< ");
        int i;
        for (i=0;i<size;i++) {
            System.out.print(bytetoint(values[i])+" ");
        }
        System.out.println("");

    }

    void dumpTransmitPacket(byte [] values, int size)
    {
        System.out.print("> ");
        int i;
        for (i=0;i<size;i++) {
            System.out.print(bytetoint(values[i])+" ");
        }
        System.out.println("");

    }
    void preProcessPacket()
    {
        int pcrc;
        /* Check CRC */
        if (pBufPtr<4) {
            /* Empty/erroneous packet */
            System.out.println("Erroneous packet - too small "+pBufPtr);
            return;
        }
        int i;
        incrc.reset();
        for (i=0;i<pBufPtr-2;i++) {
            incrc.update(pBuf[i]);
        }
        pcrc = (int)pBuf[pBufPtr-2]&0xff;
        pcrc += (pBuf[pBufPtr-1]&0xff)<<8;

        if (pcrc!=incrc.get()) {
            /* CRC error */
            System.out.println("CRC error, got " + pcrc +" expect "+incrc.get());
            return;
        }
        byte [] payload = null;

        lastPacketSize = pBufPtr-4;
        if (lastPacketSize>0) {
            payload = new byte[lastPacketSize];
            System.arraycopy(pBuf,3,payload,0,lastPacketSize);
        }

        /* Check fields */
        // pBuf[0] should be station ID
        // pBuf[1] should be control
        dumpReceivedPacket(pBuf,pBufPtr);

        if ((pBuf[1]&0x1) == 0x0) {
            // Information frame
            System.out.println("Got Information frame");
            handleInformationFrame(pBuf);
            if (lastPacketSize>0) {
                synchronized(txLock) {
                    noXmitCheck=true;
                    packetListener.handlePacket(pBuf[2],payload);
                    noXmitCheck=false;
                }
            }
        } else if ((pBuf[1]&0x3) == 0x1) {
            // Supervisory frame
            System.out.println("Got Supervisory frame");
            handleSupervisoryFrame(pBuf);
        } else {
            // Unnnumbered fram
            System.out.println("Got Unnnumbered frame");
            handleUnnumberedFrame(pBuf);
        }

        pBufPtr=0;
        checkXmit();
    }

    int bytetoint(byte b)
    {
        if (b>=0)
            return b;
        return 256+(int)b;
    }

    void ackUpTo(int seq)
    {
        seq&=0x7;
        System.out.println("Acking up to "+seq);
        seq--;
        HDLCPacket p;
        int i;
        for (i=0;i<4;seq--,i++) {
            p=ackQueue.findBySequence(seq&0x7);
            System.out.println("Trying to remove "+(seq&0x7)+" from queue");
            if (null!=p) {
                // Must be one.
                System.out.println("Removing "+(seq&0x7)+" from queue");
                ackQueue.remove(p);
                if (null!=retransmit_timer) {
                    retransmit_timer.cancel();
                    retransmit_timer=null;
                }
                return;
            }
        }
        System.out.println("********* ATTEMPTING to cancel already ack'ed frame "+(seq&0x7)+"*************");
    }

    void handleInformationFrame(byte [] buf)
    {
        // Extract seq. num.
        int seq = bytetoint(buf[1]);
        seq>>=5;
        // Ack up to seq.
        ackUpTo(seq);
    }


    void handleSupervisoryFrame(byte [] buf)
    {
        int r = bytetoint(buf[1]);
        int code = (r>>2)&0x3;
        int seq;
        switch(code) {
        case 0: // RR
            seq = r>>5;
            ackUpTo(seq);
            break;
        case 1: // RNR
            System.out.println("Got NRN frame???, disconnecting");
            disconnect();
            break;
        case 2: // REJ
            seq = r>>5;
            System.out.println("Got REJ for sequence "+seq);
            // Now, what we must do here is ask arduino to retransmit data.
            // But since it does not retransmit, we need to resend queued
            // frame.
            retransmit_queue_updated();

            break;
        default:
            System.out.println("Got unknown S frame???, disconnecting");
            disconnect();
        }
    }

    void disconnect()
    {
        HDLCPacket p = new HDLCPacket();
        p.send(output, 0x0F); // DM - Disconnect mode
        setLinkDown();
    }

    void setLinkUp()
    {
        if (null!=link_timer) {
            link_timer.cancel();
            link_timer=null;
        }
        linkUp=true;
    }

    void setLinkDown()
    {
        linkUp=false;
    }

    void handleUnnumberedFrame(byte [] buf)
    {
        // Handle at least UA (Unnumbered ack)
        //throw new java.lang.NullPointerException();

        if (buf[1]==0x63) {
            System.out.println("Link up, NRM\n");
            txSeqNum=0;
            rxSeqNum=0;
            setLinkUp();
            checkXmit();
        } else {
            System.out.println("Got unknown U frame???, disconnecting");
            disconnect();
        }
    }

    class LinkCheckTask extends TimerTask
    {
        public void run()
        {
            System.out.println("No response, retrying....");
            startLink();
        }
    }
    public void startLink()
    {
        // Send request
        HDLCPacket p = new HDLCPacket();
        p.send(output, 0x83); // SNRM - Set Normal Response Mode
        link_timer = new Timer();
        link_timer.schedule( new LinkCheckTask(), 1000 );
    }

    public void processData(byte bIn) {
        //System.out.println("Process "+(bIn&0xff));

        if (bIn==escapeFlag) {
            unEscaping=true;
            return;
        }

        // Check unescape error ?
        if (bIn==frameFlag && !unEscaping) {
            if (inPacket) {
                /* End of packet */
                if (pBufPtr>0) {
                    inPacket = false;
                    preProcessPacket();
                }
            } else {
                /* Beginning of packet */
                pBufPtr = 0;
                inPacket = true;
                incrc.reset();
            }
        } else {
            if (unEscaping) {
                bIn^=escapeXOR;
                unEscaping=false;
            }

            if (pBufPtr<MAX_PACKET_SIZE) {
                pBuf[pBufPtr++]=bIn;
            }
        }
    }


    public void addPacketListener(SerProPacketListener listener) {
        packetListener = listener;
    }


    synchronized public void serialEvent(SerialPortEvent serialEvent) {
        int numbytes,i;
        byte [] readbuf;
        if (serialEvent.getEventType() == SerialPortEvent.DATA_AVAILABLE) {
            
            try {
                while ((numbytes=input.available()) > 0) {
                    readbuf = new byte[numbytes];
                    int n = input.read(readbuf);
                    
                    for (byte b: readbuf) {
                        //System.out.println("Income: "+((int)(b&0xff)));
                        processData(b);
                    }
                }
            } catch (IOException e) {
                System.out.println("IO Exception " +e );
            }
        }
    }

    public SerProPacket createPacket()
    {
        return new HDLCPacket();
    }

    // Packet

    public class HDLCPacket implements SerProPacket {

        CRC16CCITT outcrc;

        private void sendRAW(byte b,OutputStream output)
        {
            try {
                System.out.print(bytetoint(b)+" ");
                output.write(b);
            }catch (IOException e) {
            }
        }

        private void startPacket(byte command, int payload_size) {
            outcrc.reset();
            outCommand=command;
        }


        public void sendByte(byte b,OutputStream output)
        {
            if (b==frameFlag || b==escapeFlag) {
                sendRAW(escapeFlag,output);
                sendRAW((byte)(b ^ escapeXOR),output);
            } else
                sendRAW(b,output);
        }

        public void sendData(byte [] values,OutputStream output) {
            for(byte b: values) {
                sendData(b,output);
            }
        }

        public void sendData(byte [] values,int size,OutputStream output) {
            int i;
            for(i=0;i<size;i++) {
                sendData(values[i],output);
            }
        }
        public void sendData(byte value,OutputStream output) {
            outcrc.update(value);
            sendByte(value,output);
        }

        public HDLCPacket() {
            payload = new byte[MAX_PACKET_SIZE];
            payload_size=0;
            outcrc = new CRC16CCITT();
        }

        public void append(byte b) {
            payload[payload_size++] = b;
        }
        public void append(byte [] array) {
            for (byte b:array)
                append(b);
        }

        public int getSize() {
            return payload_size;
        }

        void send(OutputStream output, int control) {
            synchronized (txLock) {
                outcrc.reset();
                System.out.print("> ");
                sendPreamble(output,control);
                if (payload_size>0)
                    sendData(payload,payload_size,output);
                sendPostamble(output);
                System.out.println("");
            }
        }

        void sendPreamble(OutputStream output,int control) {
            sendRAW(frameFlag,output);
            sendByte((byte)0xff,output); // Target
            outcrc.update((byte)0xff);
            sendByte((byte)(control&0xff),output);
            outcrc.update((byte)(control&0xff));
        }

        void sendPostamble(OutputStream output) {
            int crc = outcrc.get();
            sendByte((byte)(crc & 0xff),output);
            sendByte((byte)((crc>>8)&0xff),output);
            sendRAW(frameFlag,output);
            try {
                output.flush();
            } catch (java.io.IOException e) {
            }
        }

        public void setSeq(int s){
            seq=s;
        }
        public int getSeq() {
            return seq;
        }

        public void addU16(int value) {
            append((byte)(value&0xff));
            append((byte)((value>>>8)&0xff));
        }

        public void addS16(int value) {
            byte h = (byte)((value>>>8)&0xff);
            append((byte)(value&0xff));
            if (value<0)
                h|=0x80;
            append(h);
        }

        public void addU8(int value) {
            append((byte)(value&0xff));
        }

        public void addU8(byte value) {
            append(value);
        }

        public void addS32(int value) {
            append((byte)(value &0xff));
            append((byte)(value>>>8 &0xff));
            append((byte)(value>>>16 &0xff));
            append((byte)(value>>>24 &0xff));
        }


        public byte [] payload;
        int payload_size;
        public int seq;
    };

    // Packet queue

    public class HDLCPacketQueue {
        public HDLCPacket findBySequence(int seq) {
            System.out.println("Finding packet with sequence number "+seq);
            for (HDLCPacket p: queue) {
                System.out.println("Testing "+p.seq);
                if (p.seq==seq)
                    return p;
            }
            return null;
        }
        public void remove(HDLCPacket p)
        {
            queue.remove(p);
        }

        public int size() {
            return queue.size();
        }
        public void append(SerProPacket p)
        {
            queue.add((HDLCPacket)p);
        }
        public HDLCPacket get() {
            return queue.remove();
        }

        HDLCPacketQueue()
        {
            queue = new LinkedList<HDLCPacket>();
        }

        private LinkedList<HDLCPacket> queue;
    }

    public void queueTransmit(SerProPacket packet)
    {
        txQueue.append(packet);
        checkXmit();
    }


    void startRetransmitTimer()
    {
        if (retransmit_timer!=null)
            retransmit_timer.cancel();
        retransmit_timer=new Timer();
        retransmit_timer.schedule(new RetransmitTask(), 500);
    }

    class RetransmitTask extends TimerTask {
        public void run() {
            System.out.println("Timeout waiting for acknowledge");
            retransmit_queue();
        }
    };

    void retransmit_queue()
    {
        if (ackQueue.size()>0) {
            HDLCPacket p = ackQueue.get(); // Should be peek
            int control;
            /* Compute control */
            System.out.println("Retransmitting frame "+p.getSeq());
            control = (p.getSeq() & 0x7)<<1;
            control |= 0x10; // Poll
            p.send(output,control);
            startRetransmitTimer();
            ackQueue.append(p);
        }
    }

    void retransmit_queue_updated()
    {
        if (ackQueue.size()>0) {
            HDLCPacket p = ackQueue.get(); // Should be peek
            int control;
            /* Compute control */
            System.out.println("Retransmitting frame "+p.getSeq()+" as "+txSeqNum);
            control = (txSeqNum & 0x7)<<1;
            control |= 0x10; // Poll
            p.setSeq(txSeqNum);
            p.send(output,control);
            txSeqNum++;
            txSeqNum&=0x7;
            startRetransmitTimer();
            ackQueue.append(p);
        }
    }

    private void doTransmit(HDLCPacket p)
    {
        int control;
        /* Compute control */
        control = (txSeqNum & 0x7)<<1;
        control |= 0x10; // Poll
        p.setSeq(txSeqNum);
        txSeqNum++;
        txSeqNum&=0x7;
        ackQueue.append(p);
        p.send(output,control);
        startRetransmitTimer();
    }

    void checkXmit()
    {
        if (!linkUp || noXmitCheck)
            return;

        if (ackQueue.size()==0) {
            /* Nothing to be ack'ed */
            if (txQueue.size()>0) {
                HDLCPacket p = txQueue.get();
                doTransmit(p);
            }
        } 
    }
};
