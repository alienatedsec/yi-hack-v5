#!/bin/sh

# 0.4.1j

cd /tmp/sd/yi-hack-v5/etc

rm hostname

rm camera.conf
rm mqttv4.conf
rm proxychains.conf
rm system.conf

bzip2 -d defaults.tar.bz2
tar xvf defaults.tar > /dev/null 2>&1

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "}\n"
