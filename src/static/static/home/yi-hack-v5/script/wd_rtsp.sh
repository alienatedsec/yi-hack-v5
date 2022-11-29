#!/bin/sh

CONF_FILE="etc/system.conf"

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
MODEL_SUFFIX=$(cat /home/app/.camver)

LOG_FILE="/tmp/sd/wd_rtsp.log"
#LOG_FILE="/dev/null"

get_config()
{
    key=$1
    grep -w $1 $YI_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

COUNTER=0
COUNTER_LIMIT=10
INTERVAL=10

RRTSP_RES=$(get_config RTSP_STREAM)
RRTSP_AUDIO=$(get_config RTSP_AUDIO)
RRTSP_MODEL=$MODEL_SUFFIX
RRTSP_PORT=$(get_config RTSP_PORT)
RRTSP_USER=$USERNAME
RRTSP_PWD=$PASSWORD

restart_rtsp()
{
    rRTSPServer -r $RRTSP_RES -p $RRTSP_PORT -a $RRTSP_AUDIO &
}

restart_grabber()
{
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
}

check_rtsp()
{
    echo "$(date +'%Y-%m-%d %H:%M:%S') - Checking RTSP process..." >> $LOG_FILE
    SOCKET=`/bin/netstat -an 2>&1 | grep ":$RTSP_PORT " | grep LISTEN | grep -c ^`
    CPU=`top -b -n 1 | grep rRTSPServer | grep -v grep | tail -n 1 | awk '{print $8}'`

    if [ "$CPU" == "" ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting rRTSPServer ..." >> $LOG_FILE
        killall -q rRTSPServer
        sleep 1
        restart_rtsp
        COUNTER=0
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
           fi
        else
            COUNTER=0
        fi
    fi
}

check_rmm()
{
    echo "$(date +'%Y-%m-%d %H:%M:%S') - Checking rmm process..." >> $LOG_FILE
    PS=`ps | grep rmm | grep -v grep | grep -c ^`

    if [ $PS -eq 0 ]; then
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

if [[ $(get_config RTSP) == "no" ]] ; then
    exit
fi

if [[ "$(get_config USERNAME)" != "" ]] ; then
    USERNAME=$(get_config USERNAME)
    PASSWORD=$(get_config PASSWORD)
fi

echo "$(date +'%Y-%m-%d %H:%M:%S') - Starting RTSP watchdog..." >> $LOG_FILE

while true
do
    check_grabber
    check_rtsp
    check_rmm
    if [ $COUNTER -eq 0 ]; then
        sleep $INTERVAL
    fi
done
