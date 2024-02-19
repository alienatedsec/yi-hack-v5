#!/bin/sh

# 0.4.1j

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
ACTION="none"

removedoublequotes(){
  echo "$(sed 's/^"//g;s/"$//g')"
}

PARAM="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"
PWD=""
PWD2=""

if [ "$PARAM" == "action" ]; then
     ACTION=$VAL
fi

if [ $ACTION == "scan" ]; then

# Set content type header
printf "Content-type: application/json\r\n\r\n"

# Start JSON response
printf "{\"wifi\":["

# Scan for WiFi networks and extract ESSIDs
iwlist wlan0 scan | grep "ESSID:" | cut -d '"' -f 2 | while read -r ESSID; do
    # Check if ESSID is not empty
    if [ -n "$ESSID" ]; then
        # Print ESSID in JSON format
        printf "\"$ESSID\","
    fi
done

# Complete JSON response and remove trailing comma
printf "\"\"]}\n"

elif [ $ACTION == "save" ]; then

    read -r POST_DATA
    rm -f /tmp/configure_wifi.cfg

    KEYS=$(echo "$POST_DATA" | jq keys_unsorted[])
    for KEY in $KEYS; do
        KEY=$(echo $KEY | removedoublequotes)
        VALUE=$(echo "$POST_DATA" | jq -r .$KEY)
        if [ $KEY == "WIFI_ESSID" ]; then
            KEY="wifi_ssid"
            echo "$KEY=$VALUE" >> /tmp/configure_wifi.cfg
        elif [ $KEY == "WIFI_PASSWORD" ]; then
            PWD=$VALUE
            KEY="wifi_psk"
            echo "$KEY=$VALUE" >> /tmp/configure_wifi.cfg
        elif [ $KEY == "WIFI_PASSWORD2" ]; then
            PWD2=$VALUE
        fi
    done

    if [ "$PWD" == "$PWD2" ]; then
        $YI_HACK_PREFIX/script/configure_wifi.sh
        sleep 1
        rm -f /tmp/configure_wifi.cfg

        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "false"
        printf "}"
    else
        rm -f /tmp/configure_wifi.cfg

        printf "Content-type: application/json\r\n\r\n"
        printf "{\n"
        printf "\"%s\":\"%s\"\\n" "error" "true"
        printf "}"
    fi

fi
