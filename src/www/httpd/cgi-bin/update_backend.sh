#!/bin/sh

# 0.4.1

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

REMOTE_VERSION_FILE=/tmp/.hackremotever
REMOTE_NEWVERSION_FILE=/tmp/.hacknewver

LOCAL_VERSION_FILE=/tmp/sd/yi-hack-v5/version

IS_SD_PRESENT="NO"

if grep -qs '/tmp/sd' /proc/mounts; then
    IS_SD_PRESENT="YES"
fi

printf "Content-type: application/json\r\n\r\n"

printf "{\n"

printf "\"%s\":\"%s\",\n"  "LOCAL_VERSION" "$(cat $LOCAL_VERSION_FILE)"
printf "\"%s\":\"%s\",\n"  "REMOTE_VERSION" "$(cat $REMOTE_VERSION_FILE)"
printf "\"%s\":\"%s\",\n"  "NEEDS_UPDATE" "$(cat $REMOTE_NEWVERSION_FILE)"
printf "\"%s\":\"%s\",\n"  "IS_SD_PRESENT" "$IS_SD_PRESENT"

# Empty values to "close" the json
printf "\"%s\":\"%s\"\n"  "NULL" "NULL"

printf "}"
