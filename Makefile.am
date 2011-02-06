lib_LTLIBRARIES=libserpro-3.0.la libserpro-glib-3.0.la libserpro-linux-3.0.la


serprodir=$(includedir)/serpro-3.0/SerPro

libserpro_3_0_la_SOURCES=crc16.cpp
libserpro_3_0_la_LDFLAGS= \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= serpro-3.0.pc serpro-glib-3.0.pc serpro-linux-3.0.pc

libserpro_glib_3_0_la_SOURCES=crc16.cpp SerPro-glib.cpp
libserpro_glib_3_0_la_CXXFLAGS=@glib_CFLAGS@ -DINSERPRO -I.
libserpro_glib_3_0_la_LIBADD=@glib_LIBS@
libserpro_glib_3_0_la_LDFLAGS= \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

libserpro_linux_3_0_la_SOURCES=crc16.cpp SerPro-linux.cpp
libserpro_linux_3_0_la_CXXFLAGS=-DINSERPRO -I.
libserpro_linux_3_0_la_LDFLAGS= \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

serpro_HEADERS=crc16.h \
Packet.h               \
preprocessor_table.h   \
SerProCommon.h         \
SerPro.h               \
SerProHDLC.h           \
SerProPacket.h         \
SerPro-glib.h          \
SerPro-linux.h         
