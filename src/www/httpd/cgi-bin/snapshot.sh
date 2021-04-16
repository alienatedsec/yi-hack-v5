#!/bin/sh

validateFile()
{
    case $1 in
        *[\'!\"@\#\$%\&^*\(\),:\;]* )
            echo "invalid";;
        *)
            echo $1;;
    esac
}

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
CONF_FILE="$YI_HACK_PREFIX/etc/camera.conf"

if grep -q SWITCH_ON=no "$CONF_FILE"; then
    printf "Status: 503 Service Unavailable\r\n"
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "true"
    printf "}"
    exit 0
fi

MODEL_SUFFIX=$(cat /home/app/.camver)
BASE64="no"
RES="-r high"
WATERMARK="no"
OUTPUT_FILE="none"

for I in 1 2 3 4
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ "$CONF" == "res" ] ; then
        RES="-r $VAL"
    elif [ "$CONF" == "watermark" ] ; then
        WATERMARK=$VAL
    elif [ "$CONF" == "base64" ] ; then
        BASE64=$VAL
    elif [ "$CONF" == "file" ] ; then
        OUTPUT_FILE=$VAL
    fi
done

REDIRECT=""
if [ "$OUTPUT_FILE" != "none" ] ; then
    OUTPUT_FILE=$(validateFile "$OUTPUT_FILE")
    if [ "$OUTPUT_FILE" != "invalid" ]; then
        OUTPUT_DIR=$(cd "$(dirname "/tmp/sd/record/$OUTPUT_FILE")"; pwd)
        OUTPUT_DIR=$(echo "$OUTPUT_DIR" | cut -c1-14)
        if [ "$OUTPUT_DIR" == "/tmp/sd/record" ]; then
            REDIRECT="yes"
        fi
    fi
fi

if [ "$WATERMARK" == "yes" ] ; then
    WATERMARK="-w"
else
    WATERMARK=""
fi

if [ "$REDIRECT" == "yes" ] ; then
    if [ "$BASE64" == "no" ] ; then
        imggrabber -m $MODEL_SUFFIX $RES $WATERMARK > /tmp/sd/record/$OUTPUT_FILE
    elif [ "$BASE64" == "yes" ] ; then
        imggrabber -m $MODEL_SUFFIX $RES $WATERMARK | base64 > /tmp/sd/record/$OUTPUT_FILE
    fi
    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\"\\n" "error" "false"
    printf "}"
else
    if [ "$BASE64" == "no" ] ; then
        printf "Content-type: image/jpeg\r\n\r\n"
        imggrabber -m $MODEL_SUFFIX $RES $WATERMARK
    elif [ "$BASE64" == "yes" ] ; then
        printf "Content-type: image/jpeg;base64\r\n\r\n"
        imggrabber -m $MODEL_SUFFIX $RES $WATERMARK | base64
    fi
fi
