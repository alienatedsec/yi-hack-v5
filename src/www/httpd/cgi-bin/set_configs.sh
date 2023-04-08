#!/bin/sh
YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

sedencode(){
  echo "$(sed 's/\\/\\\\/g;s/\&/\\\&/g;s/\//\\\//g;')"
}
removedoublequotes(){
  echo "$(sed 's/^"//g;s/"$//g')"
}

get_conf_type()
{
    CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"
    if [ $CONF = "conf" ] ; then
        echo $VAL
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
ROWS=$(echo "$POST_DATA" | jq -r '. | keys[] as $k | "\($k)=\(.[$k])"')

# Create a temporary file to store the modified configuration file
TMP_CONF_FILE=$(mktemp)

# Copy the original configuration file to the temporary file
cp $CONF_FILE $TMP_CONF_FILE

# Loop through each key-value pair and update the temporary configuration file
for ROW in $ROWS; do
    KEY=$(echo $ROW | cut -d'=' -f1)
    VALUE=$(echo $ROW | cut -d'=' -f2)
    if [ "$KEY" = "HOSTNAME" ] ; then
        if [ -z $VALUE ] ; then
            # Use 2 last MAC address numbers to set a different hostname
            MAC=$(cat /sys/class/net/wlan0/address|cut -d ':' -f 5,6|sed 's/://g')
            if [ "$MAC" != "" ]; then
                hostname yi-$MAC
            else
                hostname yi-hack-v5
            fi
            hostname > $YI_HACK_PREFIX/etc/hostname
        else
            hostname $VALUE
            echo "$VALUE" > $YI_HACK_PREFIX/etc/hostname
        fi
    else
        if [ "$KEY" = "TIMEZONE" ] ; then
            echo $VALUE > /etc/TZ
        fi
        VALUE=$(echo "$VALUE" | sedencode)
        sed -i "s/^\(${KEY}[[:blank:]]*=[[:blank:]]*\).*$/\1${VALUE}/" $TMP_CONF_FILE
    fi
done

# Copy the modified temporary file back to the original configuration file
cp $TMP_CONF_FILE $CONF_FILE

# Remove the temporary file
rm $TMP_CONF_FILE

# Send a JSON response back to the client with an error flag set to `false`
printf "Content-type: application/json\r\n\r\n"
printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
