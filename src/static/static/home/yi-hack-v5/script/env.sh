#!/bin/sh

# 0.4.1i

CONF_FILE="etc/system.conf"

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

export LD_LIBRARY_PATH=/lib:/usr/lib:/home/lib:/home/app/locallib:/home/hisiko/hisilib:/tmp/sd/yi-hack-v5/lib:/home/yi-hack-v5/lib
export PATH=/usr/bin:/usr/sbin:/bin:/sbin:/home/base/tools:/home/yi-hack-v5/bin:/home/app/localbin:/home/base:/tmp/sd/yi-hack-v5/bin:/tmp/sd/yi-hack-v5/sbin:/tmp/sd/yi-hack-v5/usr/bin:/tmp/sd/yi-hack-v5/usr/sbin:/home/yi-hack-v5/sbin

get_config()
{
    key=$1
    grep -w $1 $YI_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2 | awk 'NR==1 {print; exit}'
}

TZ_CONF=$(get_config TIMEZONE)

if [ ! -z "$TZ_CONF" ]; then
    export TZ=$TZ_CONF
fi
