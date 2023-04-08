#!/bin/sh

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

sedencode(){
  echo "$(echo "$1" | sed 's/\\/\\\\/g;s/\&/\\\&/g;s/\//\\\//g;')"
}

get_conf_type()
{
    CONF="$(echo "$QUERY_STRING" | cut -d'=' -f1)"
    VAL="$(echo "$QUERY_STRING" | cut -d'=' -f2)"
    if [ "$CONF" = "conf" ] ; then
        echo "$VAL"
    fi
}

CONF_TYPE="$(get_conf_type)"
CONF_FILE=""

if [ "$CONF_TYPE" = "mqtt" ] ; then
    CONF_FILE="$YI_HACK_PREFIX/etc/mqttv4.conf"
else
    CONF_FILE="$YI_HACK_PREFIX/etc/$CONF_TYPE.conf"
fi

read -r POST_DATA
ROWS=$(echo "$POST_DATA" | awk -F '[=&]' '{for(i=1;i<=NF;i+=2) print $i "=" $(i+1)}')
for ROW in $ROWS; do
    KEY=$(echo "$ROW" | cut -d'=' -f1)
    VALUE=$(echo "$ROW" | cut -d'=' -f2)
    if [ "$KEY" = "HOSTNAME" ] ; then
        if [ -z "$VALUE" ] ; then
            # Use 2 last MAC address numbers to set a different hostname
            MAC=$(cat /sys/class/net/wlan0/address | cut -d ':' -f 5,6 | sed 's/://g')
            if [ -n "$MAC" ]; then
                hostname "yi-$MAC"
            else
                hostname "yi-hack-v5"
            fi
            echo "$(hostname)" > "$YI_HACK_PREFIX/etc/hostname"
        else
            hostname "$VALUE"
            echo "$VALUE" > "$YI_HACK_PREFIX/etc/hostname"
        fi
    else
        if [ "$KEY" = "TIMEZONE" ] ; then
            echo "$VALUE" > /etc/TZ
        fi
        VALUE=$(sedencode "$VALUE")
        sed -i "s/^\(${KEY}[[:blank:]]*=[[:blank:]]*\).*$/\1${VALUE}/" "$CONF_FILE"
    fi
    
done

printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
