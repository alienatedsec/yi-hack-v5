#!/bin/sh

# 0.4.1a

CONF_FILE="etc/system.conf"

if [ -d "/tmp/sd/yi-hack-v5" ]; then
        YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
elif [ -d "/home/yi-hack-v5" ]; then
        YI_HACK_PREFIX="/home/yi-hack-v5"
fi

YI_HACK_VER=$(cat /tmp/sd/yi-hack-v5/version)
MODEL_SUFFIX=$(cat /home/app/.camver)

YI_HACK_UPGRADE_PATH="/tmp/sd/$MODEL_SUFFIX"

get_config()
{
    key=$1
    grep -w $1 $YI_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

export LD_LIBRARY_PATH=/lib:/usr/lib:/home/lib:/home/app/locallib:/home/hisiko/hisilib:/tmp/sd/yi-hack-v5/lib:/home/yi-hack-v5/lib
export PATH=/usr/bin:/usr/sbin:/bin:/sbin:/home/base/tools:/home/yi-hack-v5/bin:/home/app/localbin:/home/base:/tmp/sd/yi-hack-v5/bin:/tmp/sd/yi-hack-v5/sbin:/tmp/sd/yi-hack-v5/usr/bin:/tmp/sd/yi-hack-v5/usr/sbin:/home/yi-hack-v5/sbin

#if [ ! -L "/var/run/utmp" ]; then
#  ln -sf /dev/null /var/run/utmp
#fi

#reversing symlinks
if [ -L "/var/run/utmp" ]; then
  rm /var/run/utmp
  reboot
fi

if [ ! -L "~/.ash_history" ]; then
  ln -sf /dev/null ~/.ash_history
fi

if [ ! -L "/home/yi-hack-v5/.ash_history" ]; then
  ln -sf /dev/null /home/yi-hack-v5/.ash_history
fi

ulimit -s 1024

# Remove core files, if any
rm -f $YI_HACK_PREFIX/bin/core
rm -f $YI_HACK_PREFIX/www/cgi-bin/core

touch /tmp/httpd.conf

if [ -f $YI_HACK_UPGRADE_PATH/yi-hack-v5/fw_upgrade_in_progress ]; then
    echo "#!/bin/sh" > /tmp/fw_upgrade_2p.sh
    echo "# Complete fw upgrade and restore configuration" >> /tmp/fw_upgrade_2p.sh
    echo "sleep 1" >> /tmp/fw_upgrade_2p.sh
    echo "cd $YI_HACK_UPGRADE_PATH" >> /tmp/fw_upgrade_2p.sh
    echo "cp -rf * .." >> /tmp/fw_upgrade_2p.sh
    echo "cd .." >> /tmp/fw_upgrade_2p.sh
    echo "rm -rf $YI_HACK_UPGRADE_PATH" >> /tmp/fw_upgrade_2p.sh
    echo "rm $YI_HACK_PREFIX/fw_upgrade_in_progress" >> /tmp/fw_upgrade_2p.sh
    echo "sync" >> /tmp/fw_upgrade_2p.sh
    echo "sync" >> /tmp/fw_upgrade_2p.sh
    echo "sync" >> /tmp/fw_upgrade_2p.sh
    echo "reboot" >> /tmp/fw_upgrade_2p.sh
    sh /tmp/fw_upgrade_2p.sh
    exit
fi

# Update cloudAPI_fake if necessary
if [[ "$(grep -m 3 -n '' /home/app/cloudAPI_fake | tail -n 1 | cut -d ':' -f 2 | cut -c 3-)" != "$(grep -m 3 -n '' $YI_HACK_PREFIX/script/cloudAPI_fake | tail -n 1 | cut -d ':' -f 2 | cut -c 3-)" ]]; then
  cp -f $YI_HACK_PREFIX/script/cloudAPI_fake /home/app/
fi

# Manual Wi-Fi config
if [ -f /tmp/sd/recover/configure_wifi.cfg ]; then
	mv /tmp/sd/recover/configure_wifi.cfg /tmp/configure_wifi.cfg
	sync
	sh $YI_HACK_PREFIX/script/configure_wifi.sh
fi

if [ -f "/tmp/sd/recover/mtdblock2_recover.bin" ]; then
	sync
	sh $YI_HACK_PREFIX/script/configure_wifi.sh
fi

$YI_HACK_PREFIX/script/check_conf.sh

hostname -F $YI_HACK_PREFIX/etc/hostname

export TZ=$(get_config TIMEZONE)

if [[ $(get_config SWAP_FILE) == "yes" ]] || [[ $MODEL_SUFFIX == "yi_dome" ]] || [[ $MODEL_SUFFIX == "yi_home" ]] ; then
    SD_PRESENT=$(mount | grep mmc | grep -c ^)
    if [[ $SD_PRESENT -ge 1 ]]; then
        if [[ -f /tmp/sd/swapfile ]]; then
            swapon /tmp/sd/swapfile
        else
            dd if=/dev/zero of=/tmp/sd/swapfile bs=1M count=64
            chmod 0600 /tmp/sd/swapfile
            mkswap /tmp/sd/swapfile
            swapon /tmp/sd/swapfile
        fi
        sysctl -w vm.dirty_background_ratio=2
        sysctl -w vm.dirty_ratio=5
        sysctl -w vm.dirty_writeback_centisecs=100
        sysctl -w vm.dirty_expire_centisecs=500
        sysctl -w vm.vfs_cache_pressure=200
        sysctl -w vm.swappiness=60
        sysctl -w vm.laptop_mode=5
    fi
fi

if [[ x$(get_config USERNAME) != "x" ]] ; then
    USERNAME=$(get_config USERNAME)
    PASSWORD=$(get_config PASSWORD)
    ONVIF_USERPWD="--user $USERNAME --password $PASSWORD"
    echo "/:$USERNAME:$PASSWORD" > /tmp/httpd.conf
fi

if [[ x$(get_config SSH_PASSWORD) != "x" ]] ; then
    SSH_PASSWORD=$(get_config SSH_PASSWORD)
#    PASSWORD_MD5="$(echo "${SSH_PASSWORD}" | mkpasswd --method=des --stdin)"
    echo root:$SSH_PASSWORD | chpasswd --md5
#    cp -f "/etc/passwd" "/tmp/sd/yi-hack-v5/etc/passwd"
#    sed -i 's|^root::|root:'${PASSWORD_MD5}':|g' "/etc/passwd"
#    sed -i 's|/root|/tmp/sd/yi-hack-v5|g' "/etc/passwd"
#    mount --bind "/tmp/sd/yi-hack-v5/etc/passwd" "/etc/passwd"
#    cp -f "/etc/shadow" "/tmp/sd/yi-hack-v5/etc/shadow"
#    sed -i 's|^root::|root:'${PASSWORD_MD5}':|g' "/etc/shadow"
#    mount --bind "/tmp/sd/yi-hack-v5/etc/shadow" "/etc/shadow"
fi

case $(get_config RTSP_PORT) in
    ''|*[!0-9]*) RTSP_PORT=554 ;;
    *) RTSP_PORT=$(get_config RTSP_PORT) ;;
esac
case $(get_config ONVIF_PORT) in
    ''|*[!0-9]*) ONVIF_PORT=80 ;;
    *) ONVIF_PORT=$(get_config ONVIF_PORT) ;;
esac
case $(get_config HTTPD_PORT) in
    ''|*[!0-9]*) HTTPD_PORT=8080 ;;
    *) HTTPD_PORT=$(get_config HTTPD_PORT) ;;
esac

# Copy library files for older versions (just in case the below firmware check is failed)
if [ ! -d "/home/yi-hack-v5/lib" ]; then
        mkdir -p "/home/yi-hack-v5/lib"
        cp -a /tmp/sd/yi-hack-v5/lib/. /home/yi-hack-v5/lib/
fi

# Version Firmware Check
if [ -f "/home/yi-hack-v5/version" ]; then
  version=$(cat /home/yi-hack-v5/version)
  if [[ "$version" != "0.3.8" ]]; then
    echo "Your baseline $version (rootfs and home partitions) is incorrect. Ignore if you are using a pre-release version. Please refer to https://github.com/alienatedsec/yi-hack-v5/discussions/267"
    echo "Your baseline $version (rootfs and home partitions) is incorrect, Ignore if you are using a pre-release version. Please refer to https://github.com/alienatedsec/yi-hack-v5/discussions/267" > "/tmp/sd/hackerror.txt"
  fi
else
  echo "Version file does not exist. Your SD card is not FAT32 to load the latest firmware correctly. Go to https://github.com/alienatedsec/yi-hack-v5/discussions/267"
  echo "Version file does not exist. Your SD card is not FAT32 to load the latest firmware correctly. Go to https://github.com/alienatedsec/yi-hack-v5/discussions/267" > "/tmp/sd/fat32error.txt"
fi

if [[ $(get_config NTPD) == "yes" ]] ; then
    # Wait until all the other processes have been initialized
    sleep 5 && ntpd -p $(get_config NTP_SERVER) &
fi

if [[ $(get_config DISABLE_CLOUD) == "no" ]] ; then
    (
        cd /home/app
        killall dispatch
        LD_PRELOAD=/home/yi-hack-v5/lib/ipc_multiplex.so ./dispatch &
        sleep 2
        LD_LIBRARY_PATH="/home/yi-hack-v5/lib:/lib:/home/lib:/home/app/locallib:/home/hisiko/hisilib" ./rmm &
        sleep 8
        ./mp4record &
        ./cloud &
        ./p2p_tnp &
        if [[ $(cat /home/app/.camver) != "yi_dome" ]] ; then
            ./oss &
        fi
        ./watch_process &
    )
fi
if [[ $(get_config DISABLE_CLOUD) == "yes" ]] ; then
    (
        cd /home/app
        killall dispatch
        LD_PRELOAD=/home/yi-hack-v5/lib/ipc_multiplex.so ./dispatch &
        sleep 2
        LD_LIBRARY_PATH="/home/yi-hack-v5/lib:/lib:/home/lib:/home/app/locallib:/home/hisiko/hisilib" ./rmm &
		sleep 8
        if [[ $(get_config REC_WITHOUT_CLOUD) == "yes" ]] ; then
            cd /home/app
            ./mp4record &
        fi
		sleep 4
        ./cloud &
    )
fi

if [[ $(get_config HTTPD) == "yes" ]] ; then
    mkdir -p /tmp/sd/record
    mkdir -p /tmp/sd/yi-hack-v5/www/record
    mount --bind /tmp/sd/record /tmp/sd/yi-hack-v5/www/record
    httpd -p $HTTPD_PORT -h $YI_HACK_PREFIX/www/ -c /tmp/httpd.conf
fi

if [[ $(get_config TELNETD) == "yes" ]] ; then
    telnetd
fi

if [[ $(get_config FTPD) == "yes" ]] ; then
    if [[ $(get_config BUSYBOX_FTPD) == "yes" ]] ; then
        tcpsvd -vE 0.0.0.0 21 ftpd -w &
    else
        pure-ftpd -B
    fi
fi

if [[ $(get_config SSHD) == "yes" ]] ; then
mkdir -p $YI_HACK_PREFIX/etc/dropbear
    if [ ! -f $YI_HACK_PREFIX/etc/dropbear/dropbear_ecdsa_host_key ]; then
        dropbearkey -t ecdsa -f /tmp/dropbear_ecdsa_host_key
        mv /tmp/dropbear_ecdsa_host_key $YI_HACK_PREFIX/etc/dropbear/
    fi
    chmod 0600 $YI_HACK_PREFIX/etc/dropbear/*
    dropbear -R -B
fi

mqttv4 &
if [[ $(get_config MQTT) == "yes" ]] ; then
    mqtt-config &
    $YI_HACK_PREFIX/script/conf2mqtt.sh &
fi

if [[ $RTSP_PORT != "554" ]] ; then
    D_RTSP_PORT=:$RTSP_PORT
fi

if [[ $HTTPD_PORT != "80" ]] ; then
    D_HTTPD_PORT=:$HTTPD_PORT
fi

if [[ $ONVIF_PORT != "80" ]] ; then
    D_ONVIF_PORT=:$ONVIF_PORT
fi

if [[ $(get_config SNAPSHOT) == "no" ]] ; then
    touch /tmp/snapshot.disabled
fi

if [[ $(get_config SNAPSHOT_LOW) == "yes" ]] ; then
    touch /tmp/snapshot.low
fi

RRTSP_MODEL=$MODEL_SUFFIX
RRTSP_RES=$(get_config RTSP_STREAM)
RRTSP_AUDIO=$(get_config RTSP_AUDIO)
RRTSP_PORT=$(get_config RTSP_PORT)
if [ ! -z $USERNAME ]; then
    RRTSP_USER="-u $USERNAME"
fi
if [ ! -z $PASSWORD ]; then
    RRTSP_PWD="-w $PASSWORD"
fi

# some non-working functions to be added later

if [[ $(get_config RTSP) == "yes" ]] ; then

    if [[ $MODEL_SUFFIX == "yi_dome" ]] || [[ $MODEL_SUFFIX == "yi_home" ]] ; then
        HIGHWIDTH="1280"
        HIGHHEIGHT="720"
    else
        HIGHWIDTH="1920"
        HIGHHEIGHT="1080"
    fi
# The below section (except ONVIF) to be also copied to service.sh
    if [[ $(get_config RTSP_AUDIO) == "yes" ]]; then
        h264grabber -r audio -m $MODEL_SUFFIX -f &
    fi
    if [[ $(get_config RTSP_STREAM) == "low" ]]; then
        h264grabber -r low -m $MODEL_SUFFIX -f &
        ONVIF_PROFILE_1="--name Profile_1 --width 640 --height 360 --url rtsp://%s$D_RTSP_PORT/ch0_1.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=low&watermark=yes --type H264"
    fi
    if [[ $(get_config RTSP_STREAM) == "high" ]]; then
        h264grabber -r high -m $MODEL_SUFFIX -f &
        ONVIF_PROFILE_0="--name Profile_0 --width $HIGHWIDTH --height $HIGHHEIGHT --url rtsp://%s$D_RTSP_PORT/ch0_0.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=high&watermark=yes --type H264"
    fi
    if [[ $(get_config RTSP_STREAM) == "both" ]]; then
         h264grabber -r low -m $MODEL_SUFFIX -f &
         h264grabber -r high -m $MODEL_SUFFIX -f &
        if [[ $(get_config ONVIF_PROFILE) == "low" ]] || [[ $(get_config ONVIF_PROFILE) == "both" ]] ; then
            ONVIF_PROFILE_1="--name Profile_1 --width 640 --height 360 --url rtsp://%s$D_RTSP_PORT/ch0_1.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=low&watermark=yes --type H264"
        fi
        if [[ $(get_config ONVIF_PROFILE) == "high" ]] || [[ $(get_config ONVIF_PROFILE) == "both" ]] ; then
            ONVIF_PROFILE_0="--name Profile_0 --width $HIGHWIDTH --height $HIGHHEIGHT --url rtsp://%s$D_RTSP_PORT/ch0_0.h264 --snapurl http://%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=high&watermark=yes --type H264"
        fi
    rRTSPServer -r $RRTSP_RES -a $RRTSP_AUDIO -p $RRTSP_PORT $RRTSP_USER $RRTSP_PWD &
    fi
#Seems to be killing the resource - fixed via #153
    $YI_HACK_PREFIX/script/wd_rtsp.sh &
#The above section (except ONVIF) to be also copied to service.sh
fi

if [[ $MODEL_SUFFIX == "yi_dome_1080p" ]] || [[ $MODEL_SUFFIX == "yi_cloud_dome_1080p" ]] ; then
    HW_ID=$(dd bs=1 count=4 skip=660 if=/tmp/mmap.info 2>/dev/null | cut -c1-4)
    SERIAL_NUMBER=$(dd bs=1 count=16 skip=664 if=/tmp/mmap.info 2>/dev/null | cut -c1-16)
else
    HW_ID=$(dd bs=1 count=4 skip=592 if=/tmp/mmap.info 2>/dev/null | cut -c1-4)
    SERIAL_NUMBER=$(dd bs=1 count=16 skip=596 if=/tmp/mmap.info 2>/dev/null | cut -c1-16)
fi

if [[ $(get_config ONVIF) == "yes" ]] ; then
    if [[ $(get_config ONVIF_NETIF) == "wlan0" ]] ; then
        ONVIF_NETIF="wlan0"
    else
        ONVIF_NETIF="eth0"
    fi

    if [[ $MODEL_SUFFIX == "yi_dome" ]] || [[ $MODEL_SUFFIX == "yi_dome_1080p" ]] || [[ $MODEL_SUFFIX == "yi_cloud_dome_1080p" ]] ; then
        onvif_srvd --pid_file /var/run/onvif_srvd.pid --model "$MODEL_SUFFIX" --manufacturer "Yi" --firmware_ver "$YI_HACK_VER" --hardware_id $HW_ID --serial_num $SERIAL_NUMBER --ifs $ONVIF_NETIF --port $ONVIF_PORT --scope onvif://www.onvif.org/Profile/S $ONVIF_PROFILE_0 $ONVIF_PROFILE_1 $ONVIF_USERPWD --ptz --move_left "/tmp/sd/yi-hack-v5/bin/ipc_cmd -m left" --move_right "/tmp/sd/yi-hack-v5/bin/ipc_cmd -m right" --move_up "/tmp/sd/yi-hack-v5/bin/ipc_cmd -m up" --move_down "/tmp/sd/yi-hack-v5/bin/ipc_cmd -m down" --move_stop "/tmp/sd/yi-hack-v5/bin/ipc_cmd -m stop" --move_preset "/tmp/sd/yi-hack-v5/bin/ipc_cmd -p %t"
    else
        onvif_srvd --pid_file /var/run/onvif_srvd.pid --model "$MODEL_SUFFIX" --manufacturer "Yi" --firmware_ver "$YI_HACK_VER" --hardware_id $HW_ID --serial_num $SERIAL_NUMBER --ifs $ONVIF_NETIF --port $ONVIF_PORT --scope onvif://www.onvif.org/Profile/S $ONVIF_PROFILE_0 $ONVIF_PROFILE_1 $ONVIF_USERPWD
    fi
    if [[ $(get_config ONVIF_WSDD) == "yes" ]] ; then
        wsdd --pid_file /var/run/wsdd.pid --if_name $ONVIF_NETIF --type tdn:NetworkVideoTransmitter --xaddr http://%s$D_ONVIF_PORT --scope "onvif://www.onvif.org/name/Unknown onvif://www.onvif.org/Profile/Streaming"
    fi
fi

framefinder $MODEL_SUFFIX &

# Add crontab
CRONTAB=$(get_config CRONTAB)
FREE_SPACE=$(get_config FREE_SPACE)
mkdir -p /var/spool/cron/crontabs/
if [ ! -z "$CRONTAB" ]; then
    echo "$CRONTAB" > /var/spool/cron/crontabs/root
fi
if [ "$FREE_SPACE" != "0" ]; then
    echo "0 * * * * /tmp/sd/yi-hack-v5/script/clean_records.sh $FREE_SPACE" > /var/spool/cron/crontabs/root
fi

/usr/sbin/crond -c /var/spool/cron/crontabs/

# Add MQTT Advertise
if [ -f "$YI_HACK_PREFIX/script/mqtt_advertise/startup.sh" ]; then
    $YI_HACK_PREFIX/script/mqtt_advertise/startup.sh
fi

# Remove log files written to SD on boot containing the WiFi password
#rm -f "/tmp/sd/log/log_first_login.tar.gz"
#rm -f "/tmp/sd/log/log_login.tar.gz"
#rm -f "/tmp/sd/log/log_p2p_clr.tar.gz"
#rm -f "/tmp/sd/log/log_wifi_connected.tar.gz"

if [[ $(get_config FTP_UPLOAD) == "yes" ]] ; then
    $YI_HACK_PREFIX/script/ftppush.sh start &
fi

if [ -f "/tmp/sd/yi-hack-v5/startup.sh" ]; then
    /tmp/sd/yi-hack-v5/startup.sh
fi

# First run on startup, then every day via crond
$YI_HACK_PREFIX/script/check_update.sh

crond -c $YI_HACK_PREFIX/etc/crontabs

