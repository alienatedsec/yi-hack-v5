#!/bin/sh

# 0.4.1

script_name=$(basename -- "$0")

if pidof "$script_name" -o $$ >/dev/null;then
   echo "Already Running - Quitting"
   exit 1
fi

CONF_FILE="etc/system.conf"

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
MODEL_SUFFIX=$(cat /home/app/.camver)

LOG_FILE="/tmp/sd/wd_rtsp.log"
#LOG_FILE="/dev/null"

get_config()
{
    key=$1
    grep -w $1 $YI_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2 | awk 'NR==1 {print; exit}'
}

COUNTER=0
COUNTER_LIMIT=10
COUNTER_STREAM=0
COUNTER_STREAM_LIMIT=3
INTERVAL=10

if [[ "$(get_config USERNAME)" != "" ]] ; then
    USERNAME=$(get_config USERNAME)
    PASSWORD=$(get_config PASSWORD)
fi

RRTSP_RES=$(get_config RTSP_STREAM)
RRTSP_AUDIO=$(get_config RTSP_AUDIO)
RRTSP_MODEL=$MODEL_SUFFIX
RRTSP_PORT=$(get_config RTSP_PORT)
if [ ! -z $USERNAME ]; then
    RRTSP_USER="-u $USERNAME"
fi
if [ ! -z $PASSWORD ]; then
    RRTSP_PWD="-w $PASSWORD"
fi

restart_rtsp()
{
    killall -q rRTSPServer
    rRTSPServer -r $RRTSP_RES -a $RRTSP_AUDIO -p $RRTSP_PORT $RRTSP_USER $RRTSP_PWD &
}

restart_grabber()
{
    killall -q rRTSPServer
    killall -q h264grabber
    if [[ $(get_config RTSP_STREAM) == "low" ]]; then
        h264grabber -r low -m $MODEL_SUFFIX -f &
    fi
    if [[ $(get_config RTSP_STREAM) == "high" ]]; then
        h264grabber -r high -m $MODEL_SUFFIX -f &
    fi
    if [[ $(get_config RTSP_STREAM) == "both" ]]; then
        h264grabber -r low -m $MODEL_SUFFIX -f &
        h264grabber -r high -m $MODEL_SUFFIX -f &
    fi
    if [[ $(get_config RTSP_AUDIO) == "yes" ]]; then
        h264grabber -r AUDIO -m $MODEL_SUFFIX -f &
    fi
    rRTSPServer -r $RRTSP_RES -a $RRTSP_AUDIO -p $RRTSP_PORT $RRTSP_USER $RRTSP_PWD &
}

restart_cloud()
{
    if [[ $(get_config DISABLE_CLOUD) == "yes" ]] ; then
    (
        cd /home/app
        ./cloud &
    )
    fi
}

restart_mqttv4()
{
    mqttv4 &
}

check_rtsp()
{
    SOCKET=`/bin/netstat -an 2>&1 | grep ":$RTSP_PORT " | grep LISTEN | grep -c ^`
    CPU=`top -b -n 1 | grep rRTSPServer | grep -v grep | tail -n 1 | awk '{print $8}'`

    if [ "$CPU" == "" ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting rRTSPServer ..." >> $LOG_FILE
        killall -q rRTSPServer
        sleep 1
        restart_rtsp
        COUNTER=0
        COUNTER_STREAM=0
    fi
    if [ $SOCKET -gt 0 ]; then
        if [ "$CPU" == "0.0" ]; then
            COUNTER=$((COUNTER+1))
            echo "$(date +'%Y-%m-%d %H:%M:%S') - Detected possible locked process ($COUNTER)" >> $LOG_FILE
            if [ $COUNTER -ge $COUNTER_LIMIT ]; then
                echo "$(date +'%Y-%m-%d %H:%M:%S') - Restarting rtsp process" >> $LOG_FILE
                killall -q rRTSPServer
                sleep 1
                restart_rtsp
                COUNTER=0
                COUNTER_STREAM=0
           fi
        else
            COUNTER=0
        fi
    fi
}

check_rtsp_stream()
{
    # Verify the RTSP data pipeline is healthy by checking that h264grabber
    # has the FIFO open for writing.  rRTSPServer opens the FIFO lazily (only
    # when a client connects), so we only check h264grabber here.
    #
    # This catches cases where h264grabber is alive but lost the FIFO fd
    # (e.g. after a shared-memory glitch from rmm).  We use "ls -la" + grep
    # because busybox on this device has no "readlink" applet.

    FIFO_NAME="h264_high_fifo"
    if [[ $(get_config RTSP_STREAM) == "low" ]]; then
        FIFO_NAME="h264_low_fifo"
    fi

    GRAB_PID=$(pidof h264grabber)

    # If h264grabber is not running, check_grabber() handles restart.
    if [ -z "$GRAB_PID" ]; then
        return
    fi

    GRAB_HAS_FIFO=$(ls -la /proc/$GRAB_PID/fd/ 2>/dev/null | grep -c "$FIFO_NAME")

    if [ "$GRAB_HAS_FIFO" -eq 0 ] 2>/dev/null; then
        COUNTER_STREAM=$((COUNTER_STREAM+1))
        echo "$(date +'%Y-%m-%d %H:%M:%S') - h264grabber missing FIFO fd ($COUNTER_STREAM/$COUNTER_STREAM_LIMIT)" >> $LOG_FILE
        if [ $COUNTER_STREAM -ge $COUNTER_STREAM_LIMIT ]; then
            echo "$(date +'%Y-%m-%d %H:%M:%S') - h264grabber FIFO broken for $COUNTER_STREAM_LIMIT checks, restarting RTSP stack ..." >> $LOG_FILE
            killall -q rRTSPServer
            killall -q h264grabber
            sleep 2
            restart_grabber
            COUNTER_STREAM=0
            COUNTER=0
        fi
    else
        COUNTER_STREAM=0
    fi
}

check_cloud()
{
    CPU=`top -b -n 1 | grep cloud | grep -v grep | tail -n 1 | awk '{print $8}'`
    if [[ $(get_config DISABLE_CLOUD) == "yes" ]] ; then
    (
    if [ "$CPU" == "" ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting ./cloud & ..." >> $LOG_FILE
        restart_cloud
        COUNTER=0
    fi
    )
    fi
}

check_rmm()
{
    PS=`ps | grep rmm | grep -v grep | grep -c ^`

    if [ $PS -eq 0 ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - ./rmm is not running, restarting the camera  ..." >> $LOG_FILE
        reboot
    fi
}

check_grabber()
{
    PS=`ps | grep h264grabber | grep -v grep | grep -c ^`

    if [ $PS -eq 0 ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting h264grabber ..." >> $LOG_FILE
        killall -q h264grabber
        sleep 1
        restart_grabber
    fi
}

check_mqttv4()
{
    PS=`ps | grep mqttv4 | grep -v grep | grep -c ^`

    if [ $PS -eq 0 ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting mqttv4 ..." >> $LOG_FILE
        killall -q mqttv4
        sleep 1
        restart_mqttv4
    fi
}

check_wifi()
{
    if [[ $(get_config WIFI_MULTI) != "yes" ]] ; then
        return
    fi

    # Check if wlan0 has an IP address
    IP_ADDR=$(ifconfig wlan0 2>/dev/null | grep "inet addr" | awk -F: '{print $2}' | awk '{print $1}')

    if [ -z "$IP_ADDR" ]; then
        WIFI_FAIL_COUNT=$((WIFI_FAIL_COUNT + 1))
        echo "$(date +'%Y-%m-%d %H:%M:%S') - WiFi has no IP address (fail count: $WIFI_FAIL_COUNT)" >> $LOG_FILE

        if [ $WIFI_FAIL_COUNT -ge 3 ]; then
            echo "$(date +'%Y-%m-%d %H:%M:%S') - WiFi connection lost, triggering wpa_supplicant reassociate..." >> $LOG_FILE

            # Check if wpa_supplicant is still running
            WPA_PS=$(ps | grep wpa_supplicant | grep -v grep | grep -c ^)
            if [ $WPA_PS -eq 0 ]; then
                echo "$(date +'%Y-%m-%d %H:%M:%S') - wpa_supplicant not running, restarting multi-WiFi..." >> $LOG_FILE
                sh $YI_HACK_PREFIX/script/wifi_connect.sh
            else
                # wpa_supplicant is running, tell it to reassociate (scan + reconnect)
                WPA_CLI="/home/base/tools/wpa_cli"
                if [ -f "$WPA_CLI" ]; then
                    $WPA_CLI -i wlan0 reassociate 2>/dev/null
                    echo "$(date +'%Y-%m-%d %H:%M:%S') - Sent reassociate command to wpa_supplicant" >> $LOG_FILE
                fi
            fi
            WIFI_FAIL_COUNT=0
        fi
    else
        WIFI_FAIL_COUNT=0
    fi
}

if [[ $(get_config RTSP) == "no" ]] ; then
    exit
fi

WIFI_FAIL_COUNT=0

# Re-enabled when its starting
echo "$(date +'%Y-%m-%d %H:%M:%S') - Starting RTSP watchdog..." >> $LOG_FILE

while true
do
    check_grabber
    check_rtsp
    check_rtsp_stream
    check_rmm
    check_cloud
    if [[ $(get_config MQTT) == "yes" ]] ; then
        check_mqttv4
    fi
    check_wifi
    if [ $COUNTER -eq 0 ]; then
        sleep $INTERVAL
    fi
done
