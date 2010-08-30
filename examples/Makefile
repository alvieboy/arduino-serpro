CXXFLAGS=-I.. $(shell pkg-config --cflags glib-2.0)
LIBS=$(shell pkg-config --libs glib-2.0)
LDFLAGS=

OBJECTS=crc16.o SerPro-glib.o

all: example1-slave example1-master

example1-slave: example1-slave.o $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

example1-master: example1-master.o $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS) -lutil
                          
SerPro-glib.o: ../SerPro-glib.cpp
	$(CXX) $(CXXFLAGS) -c  $^ -o $@ 

crc16.o: ../crc16.cpp
	$(CXX) $(CXXFLAGS) -c  $^ -o $@ 
