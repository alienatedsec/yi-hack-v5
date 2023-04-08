#!/bin/sh

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

get_conf_type()
{
    local IFS="="      # Set IFS to "=" to easily split query string
    set -- $QUERY_STRING
    [ "$1" = "conf" ] && echo "$2"
}

printf "Content-type: application/json\r\n\r\n"

CONF_TYPE="$(get_conf_type)"
CONF_FILE=""

if [ "$CONF_TYPE" = "mqtt" ] ; then
    CONF_FILE="$YI_HACK_PREFIX/etc/mqttv4.conf"
else
    CONF_FILE="$YI_HACK_PREFIX/etc/$CONF_TYPE.conf"
fi

printf "{\n"

while read -r LINE ; do
    case $LINE in
        \#*) continue ;;         # Skip comments
        *=*) KEY=${LINE%=*}      # Get key before first "="
             VALUE=${LINE#*=}    # Get value after first "="
             printf "\"%s\":\"%s\",\n" "$KEY" "${VALUE//\"/\\\"}" ;;  # Escape double quotes in value
    esac
done < "$CONF_FILE"

if [ "$CONF_TYPE" = "system" ] ; then
    HOSTNAME=$(cat "$YI_HACK_PREFIX/etc/hostname")
    printf "\"%s\":\"%s\",\n" "HOSTNAME" "${HOSTNAME//\"/\\\"}"  # Escape double quotes in hostname
fi

if [ "$CONF_TYPE" = "camera" ] ; then
    HOMEVER=$(cat /home/homever)
    printf "\"%s\":\"%s\",\n" "HOMEVER" "$HOMEVER"
fi

printf "\"%s\":\"%s\"\n" "NULL" "NULL"  # Add empty key-value pair to "close" the JSON object

printf "}"
