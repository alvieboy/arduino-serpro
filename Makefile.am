lib_LTLIBRARIES=libserpro.la libserpro-glib.la

libserpro_la_SOURCES=crc16.cpp

serprodir=$(prefix)/include/SerPro/

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= serpro.pc serpro-glib.pc

libserpro_glib_la_SOURCES=crc16.cpp SerPro-glib.cpp
libserpro_glib_la_CXXFLAGS=@glib_CFLAGS@ -DINSERPRO -I.
libserpro_glib_la_LIBADD=@glib_LIBS@


serpro_HEADERS=crc16.h \
Packet.h               \
preprocessor_table.h   \
SerProCommon.h         \
SerPro.h               \
SerProHDLC.h           \
SerProPacket.h         \
SerPro-glib.h
