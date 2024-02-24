#!/bin/sh

# 0.4.1

killall udhcpc
HN="yi-hack-v5"
if [ -f /tmp/sd/yi-hack-v5/etc/hostname ]; then
        HN=$(cat /tmp/sd/yi-hack-v5/etc/hostname)
fi
/sbin/udhcpc -i wlan0 -b -s /home/app/script/default.script -x hostname:$HN
