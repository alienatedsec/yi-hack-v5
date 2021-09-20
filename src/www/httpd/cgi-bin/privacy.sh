#!/bin/sh

CONF_FILE="etc/system.conf"

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

YI_HACK_VER=$(cat /tmp/sd/yi-hack-v5/version)
MODEL_SUFFIX=$(cat /home/app/.camver)
if [[ $MODEL_SUFFIX == "yi_dome_1080p" ]] || [[ $MODEL_SUFFIX == "yi_cloud_dome_1080p" ]] ; then
    HW_ID=$(dd bs=1 count=4 skip=660 if=/tmp/mmap.info 2>/dev/null | cut -c1-4)
    SERIAL_NUMBER=$(dd bs=1 count=16 skip=664 if=/tmp/mmap.info 2>/dev/null | cut -c1-16)
else
    HW_ID=$(dd bs=1 count=4 skip=592 if=/tmp/mmap.info 2>/dev/null | cut -c1-4)
    SERIAL_NUMBER=$(dd bs=1 count=16 skip=596 if=/tmp/mmap.info 2>/dev/null | cut -c1-16)
fi

get_config()
{
    key=$1
    grep -w $1 $YI_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2
}

init_config()
{
    if [[ x$(get_config USERNAME) != "x" ]] ; then
        USERNAME=$(get_config USERNAME)
        PASSWORD=$(get_config PASSWORD)
    fi

    case $(get_config RTSP_PORT) in
        ''|*[!0-9]*) RTSP_PORT=554 ;;
        *) RTSP_PORT=$(get_config RTSP_PORT) ;;
    esac

    if [[ $RTSP_PORT != "554" ]] ; then
        D_RTSP_PORT=:$RTSP_PORT
    fi
}

start_rtsp()
{
    RRTSP_MODEL=$MODEL_SUFFIX
    RRTSP_RES=$(get_config RTSP_STREAM)
    RRTSP_AUDIO=$(get_config RTSP_AUDIO)
    RRTSP_PORT=$RTSP_PORT
    RRTSP_USER=$USERNAME
    RRTSP_PWD=$PASSWORD

    h264grabber -r $RRTSP_RES -m $MODEL_SUFFIX -f &
    rRTSPServer -r $RRTSP_RES -p $RRTSP_PORT &
    $YI_HACK_PREFIX/script/wd_rtsp.sh >/dev/null &
}

stop_rtsp()
{
    killall wd_rtsp.sh
    killall rRTSPServer
}

ps_program()
{
    PS_PROGRAM=$(ps | grep $1 | grep -v grep | grep -c ^)
    if [ $PS_PROGRAM -gt 0 ]; then
        echo "started"
    else
        echo "stopped"
    fi
}

VALUE="none"
RES="none"

CONF="$(echo $QUERY_STRING | cut -d'&' -f1 | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'&' -f1 | cut -d'=' -f2)"

if [ "$CONF" == "value" ] ; then
    VALUE="$VAL"
fi

init_config

if [ "$VALUE" == "on" ] ; then
    touch /tmp/privacy
    touch /tmp/snapshot.disabled
    stop_rtsp
    killall mp4record
    RES="on"
elif [ "$VALUE" == "off" ] ; then
    rm -f /tmp/snapshot.disabled
    start_rtsp
    cd /home/app
    ./mp4record >/dev/null &
    rm -f /tmp/privacy
    RES="off"
elif [ "$VALUE" == "status" ] ; then
    if [ -f /tmp/privacy ]; then
        RES="on"
    else
        RES="off"
    fi
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
if [ ! -z "$RES" ]; then
    printf "\"status\": \"$RES\"\n"
fi
printf "}"
