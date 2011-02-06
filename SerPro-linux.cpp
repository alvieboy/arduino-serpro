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

#include <SerPro-linux.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define OUTBUFSIZE 8192
static char strbuffer[OUTBUFSIZE];
static char *strbufferptr = &strbuffer[0];

void SerialWrapper::write(uint8_t v) {
    *strbufferptr++=v;
}

void SerialWrapper::write(const unsigned char *buf, unsigned int size) {
	memcpy(strbufferptr,buf,size);
	strbufferptr+=size;
}

void SerialWrapper::flush() {
	::write(SerProLinux::fd,strbuffer,strbufferptr-strbuffer);
	strbufferptr=&strbuffer[0];
}

bool SerialWrapper::waitEvents(bool block) {
	//return g_main_context_iteration(g_main_context_default(), TRUE);
	SerProLinux::loop();
}

LinuxTimer::timer_t LinuxTimer::addTimer( int (*cb)(void*), int milisseconds, void *data)
{
	//return g_timeout_add(milisseconds,cb,data);
	struct timeval now;
	struct timeval delta;
	TimerEntry e;
	e.data = data;
	e.callback= (bool(*)(void*))cb;
	e.id = SerProLinux::ids++;

	gettimeofday(&now,NULL);
	delta.tv_usec = (milisseconds*1000)%1000000;
	delta.tv_sec = (milisseconds/1000);
	timeradd(&now,&delta,&e.expires);
//	fprintf(stderr,"Add new timer %d, cb %p\n", e.id, e.callback);
	SerProLinux::events.insert(e);
	return e.id;
}

LinuxTimer::timer_t LinuxTimer::cancelTimer(const timer_t &t)
{
//	fprintf(stderr,"Cancelling timer %d\n",t);

	std::set<TimerEntry,TimerEntry>::iterator nexttimer;

	for (nexttimer=SerProLinux::events.begin();
		 nexttimer!=SerProLinux::events.end();
		 nexttimer++) {
		if (nexttimer->id == t) {
			SerProLinux::events.erase(nexttimer);
			return 0;
		}
	}
  //  fprintf(stderr,"ERROR: timer not found!\n");
	return 0;
}

int SerProLinux::init(const char *device, speed_t baudrate)
{
	fd = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK);
	if (fd<0) {
		perror("open");
		return -1;
	}
	return init(fd,baudrate);
}

int SerProLinux::init(int fdpty, speed_t baudrate)
{
	struct termios termset;
	int status;

	fd=fdpty;

	tcgetattr(fd, &termset);
	termset.c_iflag = IGNBRK;
	cfmakeraw(&termset);
	termset.c_oflag &= ~OPOST;
	termset.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termset.c_cflag &= ~(CSIZE | PARENB| HUPCL);
	termset.c_cflag |= CS8;
	termset.c_cc[VMIN]=1;
	termset.c_cc[VTIME]=5;

	cfsetospeed(&termset,baudrate);
	cfsetispeed(&termset,baudrate);

	tcsetattr(fd,TCSANOW,&termset);

	ioctl(fd, TIOCMGET, &status); 

	status |= ( TIOCM_DTR | TIOCM_RTS );

	ioctl(fd, TIOCMSET, &status);


	//g_io_channel_set_buffered(channel, true);

	if (fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK)<0) {
		fprintf(stderr,"Cannot set flags: %s\n", strerror(errno));
	}

	in_request = false;
	delay_request = false;

	return 0;
}

void SerProLinux::loop()
{
	fd_set rfs;
	struct timeval tv,now;
	bool run_timer = false;
	std::set<TimerEntry,TimerEntry>::iterator nexttimer;

	FD_ZERO(&rfs);
	int r;

	FD_SET(fd, &rfs);

	if (events.empty()) {
		tv.tv_sec=1;
		tv.tv_usec=0;
	} else {
		nexttimer = events.begin();
		gettimeofday(&now,NULL);
		if (nexttimer->expired(now)) {
			fprintf(stderr,"Timer %d expired\n", nexttimer->id);
			run_timer = true;
		} else {
			nexttimer->expires_in(now,tv);
		}
	}

	switch(r=select(fd+1,&rfs,NULL,NULL,&tv)) {
	case 0:
		break;
	case -1:
		throw 1;
		break;
	default:
		serial_data_ready();
		break;
	}
	if (run_timer) {
		TimerEntry e = *nexttimer;
		events.erase(nexttimer);
		fprintf(stderr,"Calling timer %d %p\n",e.id,e.callback);
		e.callback(e.data);
	}

}

void SerProLinux::run()
{
	loop();
}

void SerProLinux::runEvents()
{
	loop();
}

void SerProLinux::waitConnection()
{
	do {
		SerProLinux::runEvents();
	} while (!connected);

}

bool SerProLinux::serial_data_ready()
{
	int r,i;
	char c[8192];

	r = read(fd,c,8192);
	if ((r==-1) && errno==EAGAIN)
		return true;

	if (r>0)  {
		for (i=0;i<r;i++) {
			process((unsigned char)c[i]);
		}
	}
	return true;
}

void SerProLinux::onConnect(void (*func)(void))
{
	connectCB = func;

}
int connectEventImpl(void* arg)
{
	void (*func)(void) = (void(*)(void))arg;
	if (func)
		func();
	return false;
}

void SerProLinux::connectEvent() {
	connected = true;
	//g_timeout_add( 0, &connectEventImpl, (void*)connectCB);
}


int SerProLinux::fd;
bool SerProLinux::in_request, SerProLinux::delay_request;
void (*SerProLinux::connectCB)() = NULL;
bool SerProLinux::connected=false;
int SerProLinux::ids=1;
std::set<TimerEntry,TimerEntry> SerProLinux::events;
