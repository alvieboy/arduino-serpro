#!/bin/sh

LIBFILES_1="SerProArduino.h crc16.cpp"

LIBFILES_2="crc16.h Packet.h SerProCommon.h SerProHDLC.h IPStack.h  preprocessor_table.h SerProArduino.h  SerPro.h SerProPacket.h"

tmpdir=`mktemp -d`;
[ -d ${tmpdir} ] || exit -1
CWD=`pwd`;
[ -z ${CWD} ] && exit -1

mkdir -p ${tmpdir}/SerPro/serpro || exit -1

for file in ${LIBFILES_1}; do 
	install -m 644 -D $file ${tmpdir}/SerPro || exit -1
done;

for file in ${LIBFILES_2}; do 
	install -m 644 -D $file ${tmpdir}/SerPro/serpro/ || exit -1
done; 

cd ${tmpdir} && tar cfvz ${CWD}/SerPro.tar.gz SerPro;
cd ${CWD};

rm -rf ${tmpdir}