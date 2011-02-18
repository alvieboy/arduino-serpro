CXXFLAGS=$(shell pkg-config --cflags glib-2.0 serpro-glib-3.0)
LIBS=$(shell pkg-config --libs glib-2.0 serpro-glib-3.0)
LDFLAGS=

BINS=example1-slave example1-master example-arduino

all: $(BINS)

example1-slave: example1-slave.o $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

example1-master: example1-master.o $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS) -lutil
                          
example-arduino: example-arduino.o $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS) -lutil
                          
clean:
	rm -f $(BINS) *.o
	