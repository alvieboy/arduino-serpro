lib_LTLIBRARIES=libserpro.la libserpro-glib.la libserpro-linux.la

libserpro_la_SOURCES=crc16.cpp

serprodir=$(prefix)/include/SerPro/

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= serpro.pc serpro-glib.pc serpro-linux.pc

libserpro_glib_la_SOURCES=crc16.cpp SerPro-glib.cpp
libserpro_glib_la_CXXFLAGS=@glib_CFLAGS@ -DINSERPRO -I.
libserpro_glib_la_LIBADD=@glib_LIBS@

libserpro_linux_la_SOURCES=crc16.cpp SerPro-linux.cpp
libserpro_linux_la_CXXFLAGS=-DINSERPRO -I.

serpro_HEADERS=crc16.h \
Packet.h               \
preprocessor_table.h   \
SerProCommon.h         \
SerPro.h               \
SerProHDLC.h           \
SerProPacket.h         \
SerPro-glib.h          \
SerPro-linux.h         
