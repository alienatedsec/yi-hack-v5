#!/bin/sh

# 0.4.1

CONF_FILE="etc/system.conf"

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

get_config()
{
    key=$1
    grep -w $1 $YI_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2 | awk 'NR==1 {print; exit}'
}

LOCAL_IP_WLAN=$(ifconfig wlan0 | awk '/inet addr/{print substr($2,6)}')
LOCAL_IP_ETH=$(ifconfig eth0 | awk '/inet addr/{print substr($2,6)}')

if [[ $LOCAL_IP_WLAN != "" ]] ; then
    LOCAL_IP=$LOCAL_IP_WLAN
elif [[ $LOCAL_IP_ETH != "" ]] ; then
    LOCAL_IP=$LOCAL_IP_ETH
elif [[ $LOCAL_IP_WLAN == "" ]] && [[ $LOCAL_IP_ETH == "" ]] ; then
    LOCAL_IP="127.0.0.1"
fi

case $(get_config RTSP_PORT) in
    ''|*[!0-9]*) RTSP_PORT=554 ;;
    *) RTSP_PORT=$(get_config RTSP_PORT) ;;
esac

if [[ $RTSP_PORT != "554" ]] ; then
    D_RTSP_PORT=:$RTSP_PORT
fi

# Host the client used to reach us, so that links work when the camera is
# accessed via a different name (e.g. an NGINX tunnel domain) - #255.
# The HTTP_HOST header may carry a port (Host: 192.168.x.x:8080); strip it
# for the RTSP URLs below, which get their own (RTSP_PORT) appended.
LINK_HOST=$HTTP_HOST
if [[ $LINK_HOST == "" ]] ; then
    LINK_HOST=$LOCAL_IP
fi
LINK_HOST=${LINK_HOST%%:*}

printf "Content-type: application/json\r\n\r\n"
printf "{\n"

if [[ $(get_config RTSP) == "yes" ]] ; then
    if [[ $(get_config RTSP_STREAM) == "low" ]] ; then
        printf "\"%s\":\"%s\",\n" "low_res_stream"        "rtsp://$LINK_HOST$D_RTSP_PORT/ch0_1.h264"
    elif [[ $(get_config RTSP_STREAM) == "high" ]] ; then
        printf "\"%s\":\"%s\",\n" "high_res_stream"       "rtsp://$LINK_HOST$D_RTSP_PORT/ch0_0.h264"
    elif [[ $(get_config RTSP_STREAM) == "both" ]] ; then
        printf "\"%s\":\"%s\",\n" "low_res_stream"        "rtsp://$LINK_HOST$D_RTSP_PORT/ch0_1.h264"
        printf "\"%s\":\"%s\",\n" "high_res_stream"       "rtsp://$LINK_HOST$D_RTSP_PORT/ch0_0.h264"
    fi
    if [[ $(get_config RTSP_AUDIO) != "no" ]] && [[ $(get_config RTSP_AUDIO) != "none" ]] ; then
        printf "\"%s\":\"%s\",\n" "audio_stream"        "rtsp://$LINK_HOST$D_RTSP_PORT/ch0_2.h264"
    fi
fi

# Same-origin resources: root-relative paths (menu-bar style) so they resolve
# against whatever host:port the browser is already using - #255.
printf "\"%s\":\"%s\",\n" "low_res_snapshot"      "/cgi-bin/snapshot.sh?res=low&watermark=yes"
printf "\"%s\":\"%s\"\n" "high_res_snapshot"      "/cgi-bin/snapshot.sh?res=high&watermark=yes"

printf "}"
