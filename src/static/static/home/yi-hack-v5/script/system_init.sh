#!/bin/sh

# 0.4.1j

if [ -d "/usr/yi-hack-v5" ]; then
    YI_HACK_V5_PREFIX="/usr"
    YI_PREFIX="/home"
    UDHCPC_SCRIPT_DEST="/home/default.script"
elif [ -d "/home/yi-hack-v5" ]; then
    YI_HACK_V5_PREFIX="/home"
    YI_PREFIX="/home/app"
    YI_BASE="/home/base/tools"
    YI_LIB="/home/lib"
    UDHCPC_SCRIPT_DEST="/home/app/script/default.script"
fi

ARCHIVE_FILE="$YI_HACK_V5_PREFIX/yi-hack-v5/yi-hack-v5.7z"
DESTDIR="$YI_HACK_V5_PREFIX/yi-hack-v5"

DHCP_SCRIPT_DEST="/home/app/script/wifidhcp.sh"
UDHCP_SCRIPT="$YI_HACK_V5_PREFIX/yi-hack-v5/script/default.script"
DHCP_SCRIPT="$YI_HACK_V5_PREFIX/yi-hack-v5/script/wifidhcp.sh"

files=`find $YI_PREFIX -maxdepth 1 -name "*.7z" | awk 'END { print NR }'`
if [ $files -gt 0 ]; then
	/home/base/tools/7za x "$YI_PREFIX/*.7z" -y -o$YI_PREFIX
	rm $YI_PREFIX/*.7z
fi

files=`find $YI_BASE -maxdepth 1 -name "*.7z" | awk 'END { print NR }'`
if [ $files -gt 0 ]; then
	/home/base/tools/7za x "$YI_BASE/*.7z" -y -o$YI_BASE
	rm $YI_BASE/*.7z
fi

files=`find $YI_LIB -maxdepth 1 -name "*.7z" | awk 'END { print NR }'`
if [ $files -gt 0 ]; then
	/home/base/tools/7za x "$YI_LIB/*.7z" -y -o$YI_LIB
	rm $YI_LIB/*.7z
fi

if [ -f $ARCHIVE_FILE ]; then
	/home/base/tools/7za x $ARCHIVE_FILE -y -o$DESTDIR
	rm $ARCHIVE_FILE
fi

if [ ! -f $YI_PREFIX/cloudAPI_real ]; then
	mv $YI_PREFIX/cloudAPI $YI_PREFIX/cloudAPI_real
	cp $YI_HACK_V5_PREFIX/yi-hack-v5/script/cloudAPI $YI_PREFIX/
	cp $YI_HACK_V5_PREFIX/yi-hack-v5/script/cloudAPI_fake $YI_PREFIX/
        rm $UDHCPC_SCRIPT_DEST
        cp $UDHCP_SCRIPT $UDHCPC_SCRIPT_DEST
	if [ -f $DHCP_SCRIPT_DEST ]; then
		rm $DHCP_SCRIPT_DEST
		cp $DHCP_SCRIPT $DHCP_SCRIPT_DEST
	fi
fi

mkdir -p $YI_HACK_V5_PREFIX/yi-hack-v5/etc/crontabs
mkdir -p $YI_HACK_V5_PREFIX/yi-hack-v5/etc/dropbear

# Comment out all the cloud stuff from base/init.sh
sed -i '/^\.\/dispatch/s/^/#/' /home/app/init.sh
sed -i '/^\.\/watch_process/s/^/#/' /home/app/init.sh
sed -i '/^\.\/oss/s/^/#/' /home/app/init.sh
sed -i '/^\.\/p2p_tnp/s/^/#/' /home/app/init.sh
sed -i '/^\.\/cloud/s/^/#/' /home/app/init.sh
sed -i '/^\.\/mp4record/s/^/#/' /home/app/init.sh
sed -i '/^\.\/rmm/s/^/#/' /home/app/init.sh
sed -i 's|^sleep 2$|#sleep 2|' /home/app/init.sh
# set swappiness from 0 to 60
sed -i "s#echo 0 > /proc/sys/vm/swappiness#echo 60 > /proc/sys/vm/swappiness#" /home/app/init.sh

# Comment out the rtc command that sometimes hangs the camera in base/init.sh
# rtctime=$(/home/base/tools/rtctool -g time
# date -s $rtctime
sed -i '/^rtctime=\$(\/home\/base\/tools\/rtctool -g time)/s/^/#/' /home/base/init.sh
sed -i '/^date -s \$rtctime/s/^/#/' /home/base/init.sh
