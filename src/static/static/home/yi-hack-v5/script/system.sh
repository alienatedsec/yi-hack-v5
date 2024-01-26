#!/bin/sh

# 0.4.0c

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
    RTSP_USERPWD=$USERNAME:$PASSWORD@
    ONVIF_USERPWD="user=$USERNAME\npassword=$PASSWORD"
    echo "/onvif::" > /tmp/httpd.conf
    echo "/:$USERNAME:$PASSWORD" >> /tmp/httpd.conf
    chmod 0600 /tmp/httpd.conf
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
case $(get_config HTTPD_PORT) in
    ''|*[!0-9]*) HTTPD_PORT=80 ;;
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

ipc_multiplexer &

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

if [[ $(get_config ONVIF_WM_SNAPSHOT) == "yes" ]] ; then
    WATERMARK="&watermark=yes"
fi

RRTSP_MODEL=$MODEL_SUFFIX
RRTSP_RES=$(get_config RTSP_STREAM)
RRTSP_AUDIO=$(get_config RTSP_AUDIO)
RRTSP_PORT=$(get_config RTSP_PORT)
RRTSP_USER=$USERNAME
RRTSP_PWD=$PASSWORD

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
        ONVIF_PROFILE_1="name=Profile_1\nwidth=640\nheight=360\nurl=rtsp://$RTSP_USERPWD%s$D_RTSP_PORT/ch0_1.h264\nsnapurl=http://$RTSP_USERPWD%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=low$WATERMARK\ntype=H264"
    fi
    if [[ $(get_config RTSP_STREAM) == "high" ]]; then
        h264grabber -r high -m $MODEL_SUFFIX -f &
        ONVIF_PROFILE_0="name=Profile_0\nwidth=$HIGHWIDTH\nheight=$HIGHHEIGHT\nurl=rtsp://$RTSP_USERPWD%s$D_RTSP_PORT/ch0_0.h264\nsnapurl=http://$RTSP_USERPWD%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=high$WATERMARK\ntype=H264"
    fi
    if [[ $(get_config RTSP_STREAM) == "both" ]]; then
         h264grabber -r low -m $MODEL_SUFFIX -f &
         h264grabber -r high -m $MODEL_SUFFIX -f &
        if [[ $(get_config ONVIF_PROFILE) == "low" ]] || [[ $(get_config ONVIF_PROFILE) == "both" ]] ; then
            ONVIF_PROFILE_1="name=Profile_1\nwidth=640\nheight=360\nurl=rtsp://$RTSP_USERPWD%s$D_RTSP_PORT/ch0_1.h264\nsnapurl=http://$RTSP_USERPWD%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=low$WATERMARK\ntype=H264"
        fi
        if [[ $(get_config ONVIF_PROFILE) == "high" ]] || [[ $(get_config ONVIF_PROFILE) == "both" ]] ; then
            ONVIF_PROFILE_0="name=Profile_0\nwidth=$HIGHWIDTH\nheight=$HIGHHEIGHT\nurl=rtsp://$RTSP_USERPWD%s$D_RTSP_PORT/ch0_0.h264\nsnapurl=http://$RTSP_USERPWD%s$D_HTTPD_PORT/cgi-bin/snapshot.sh?res=high$WATERMARK\ntype=H264"
        fi
    rRTSPServer -r $RRTSP_RES -a $RRTSP_AUDIO -p $RRTSP_PORT -u $RRTSP_USER -w $RRTSP_PWD &
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

    ONVIF_SRVD_CONF="/tmp/onvif_simple_server.conf"

    echo "model=Yi Hack" > $ONVIF_SRVD_CONF
    echo "manufacturer=Yi" >> $ONVIF_SRVD_CONF
    echo "firmware_ver=$YI_HACK_VER" >> $ONVIF_SRVD_CONF
    echo "hardware_id=$HW_ID" >> $ONVIF_SRVD_CONF
    echo "serial_num=$SERIAL_NUMBER" >> $ONVIF_SRVD_CONF
    echo "ifs=$ONVIF_NETIF" >> $ONVIF_SRVD_CONF
    echo "port=$HTTPD_PORT" >> $ONVIF_SRVD_CONF
    echo "scope=onvif://www.onvif.org/Profile/Streaming" >> $ONVIF_SRVD_CONF
    echo "" >> $ONVIF_SRVD_CONF
    if [ ! -z $ONVIF_USERPWD ]; then
        echo -e $ONVIF_USERPWD >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi
    if [ ! -z $ONVIF_PROFILE_0 ]; then
        echo "#Profile 0" >> $ONVIF_SRVD_CONF
        echo -e $ONVIF_PROFILE_0 >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi
    if [ ! -z $ONVIF_PROFILE_1 ]; then
        echo "#Profile 1" >> $ONVIF_SRVD_CONF
        echo -e $ONVIF_PROFILE_1 >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi

    if [[ $MODEL_SUFFIX == "yi_dome" ]] || [[ $MODEL_SUFFIX == "yi_dome_1080p" ]] || [[ $MODEL_SUFFIX == "yi_cloud_dome_1080p" ]] ; then
        echo "#PTZ" >> $ONVIF_SRVD_CONF
        echo "ptz=1" >> $ONVIF_SRVD_CONF
        echo "get_position=/tmp/sd/yi-hack-v5/bin/ipc_cmd -g" >> $ONVIF_SRVD_CONF
        echo "move_left=/tmp/sd/yi-hack-v5/bin/ipc_cmd -m left" >> $ONVIF_SRVD_CONF
        echo "move_right=/tmp/sd/yi-hack-v5/bin/ipc_cmd -m right" >> $ONVIF_SRVD_CONF
        echo "move_up=/tmp/sd/yi-hack-v5/bin/ipc_cmd -m up" >> $ONVIF_SRVD_CONF
        echo "move_down=/tmp/sd/yi-hack-v5/bin/ipc_cmd -m down" >> $ONVIF_SRVD_CONF
        echo "move_stop=/tmp/sd/yi-hack-v5/bin/ipc_cmd -m stop" >> $ONVIF_SRVD_CONF
        echo "move_preset=/tmp/sd/yi-hack-v5/bin/ipc_cmd -p %d" >> $ONVIF_SRVD_CONF
        echo "set_preset=/tmp/sd/yi-hack-v5/script/ptz_presets.sh -a add_preset -m %s" >> $ONVIF_SRVD_CONF
        echo "set_home_position=/tmp/sd/yi-hack-v5/script/ptz_presets.sh -a set_home_position" >> $ONVIF_SRVD_CONF
        echo "remove_preset=/tmp/sd/yi-hack-v5/script/ptz_presets.sh -a del_preset -n %d" >> $ONVIF_SRVD_CONF
        echo "jump_to_abs=/tmp/sd/yi-hack-v5/bin/ipc_cmd -j %f,%f" >> $ONVIF_SRVD_CONF
        echo "jump_to_rel=/tmp/sd/yi-hack-v5/bin/ipc_cmd -J %f,%f" >> $ONVIF_SRVD_CONF
        echo "get_presets=/tmp/sd/yi-hack-v5/script/ptz_presets.sh -a get_presets" >> $ONVIF_SRVD_CONF
        echo "" >> $ONVIF_SRVD_CONF
    fi

    echo "#EVENT" >> $ONVIF_SRVD_CONF
    echo "events=3" >> $ONVIF_SRVD_CONF
    echo "#Event 0" >> $ONVIF_SRVD_CONF
    echo "topic=tns1:VideoSource/MotionAlarm" >> $ONVIF_SRVD_CONF
    echo "source_name=VideoSourceConfigurationToken" >> $ONVIF_SRVD_CONF
    echo "source_value=VideoSourceToken" >> $ONVIF_SRVD_CONF
    echo "input_file=/tmp/onvif_notify_server/motion_alarm" >> $ONVIF_SRVD_CONF
    echo "#Event 1" >> $ONVIF_SRVD_CONF
    echo "topic=tns1:RuleEngine/MyRuleDetector/PeopleDetect" >> $ONVIF_SRVD_CONF
    echo "source_name=VideoSourceConfigurationToken" >> $ONVIF_SRVD_CONF
    echo "source_value=VideoSourceToken" >> $ONVIF_SRVD_CONF
    echo "input_file=/tmp/onvif_notify_server/human_detection" >> $ONVIF_SRVD_CONF
    echo "#Event 2" >> $ONVIF_SRVD_CONF
    echo "topic=tns1:RuleEngine/MyRuleDetector/VehicleDetect" >> $ONVIF_SRVD_CONF
    echo "source_name=VideoSourceConfigurationToken" >> $ONVIF_SRVD_CONF
    echo "source_value=VideoSourceToken" >> $ONVIF_SRVD_CONF
    echo "input_file=/tmp/onvif_notify_server/vehicle_detection" >> $ONVIF_SRVD_CONF
    echo "#Event 3" >> $ONVIF_SRVD_CONF
    echo "topic=tns1:RuleEngine/MyRuleDetector/DogCatDetect" >> $ONVIF_SRVD_CONF
    echo "source_name=VideoSourceConfigurationToken" >> $ONVIF_SRVD_CONF
    echo "source_value=VideoSourceToken" >> $ONVIF_SRVD_CONF
    echo "input_file=/tmp/onvif_notify_server/animal_detection" >> $ONVIF_SRVD_CONF
    echo "#Event 4" >> $ONVIF_SRVD_CONF
    echo "topic=tns1:RuleEngine/MyRuleDetector/BabyCryingDetect" >> $ONVIF_SRVD_CONF
    echo "source_name=VideoSourceConfigurationToken" >> $ONVIF_SRVD_CONF
    echo "source_value=VideoSourceToken" >> $ONVIF_SRVD_CONF
    echo "input_file=/tmp/onvif_notify_server/baby_crying" >> $ONVIF_SRVD_CONF
    echo "#Event 5" >> $ONVIF_SRVD_CONF
    echo "topic=tns1:AudioAnalytics/Audio/DetectedSound" >> $ONVIF_SRVD_CONF
    echo "source_name=VideoSourceConfigurationToken" >> $ONVIF_SRVD_CONF
    echo "source_value=VideoSourceToken" >> $ONVIF_SRVD_CONF
    echo "input_file=/tmp/onvif_notify_server/sound_detection" >> $ONVIF_SRVD_CONF

    chmod 0600 $ONVIF_SRVD_CONF
    onvif_simple_server --conf_file $ONVIF_SRVD_CONF
    ipc2file
    onvif_notify_server --conf_file $ONVIF_SRVD_CONF

    if [[ $(get_config ONVIF_WSDD) == "yes" ]] ; then
        wsd_simple_server --pid_file /var/run/wsd_simple_server.pid --if_name $ONVIF_NETIF --xaddr "http://%s$D_HTTPD_PORT/onvif/device_service" -m yi_hack -n Yi
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

