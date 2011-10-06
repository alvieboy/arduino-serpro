#!/bin/sh
set -e

LIBFILES_1="crc16.cpp"

LIBFILES_2="crc16.h Packet.h SerProCommon.h SerProHDLC.h  preprocessor_table.h SerProArduino.h  SerPro.h SerProPacket.h"

tmpdir=`mktemp -d`;
[ -d ${tmpdir} ] || exit -1
CWD=`pwd`;
[ -z ${CWD} ] && exit -1

mkdir -p ${tmpdir}/SerPro3/serpro || exit -1

for file in ${LIBFILES_1}; do 
	install -m 644 -D $file ${tmpdir}/SerPro3 || exit -1
done;

install -m 644 SerProArduino.h ${tmpdir}/SerPro3/SerPro3.h  || exit

for file in ${LIBFILES_2}; do 
	install -m 644 -D $file ${tmpdir}/SerPro3/serpro/ || exit -1
done; 

cd ${tmpdir} && tar cfz ${CWD}/SerPro3.tar.gz SerPro3;
cd ${CWD};

rm -rf ${tmpdir}

echo "Created SerPro3.tar.gz"
