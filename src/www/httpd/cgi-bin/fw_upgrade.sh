#!/bin/sh

# 0.4.1

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
MODEL_SUFFIX=`cat /home/app/.camver`
FW_VERSION=`cat /tmp/sd/yi-hack-v5/version`
BASELINE_VERSION=`cat /home/yi-hack-v5/version`

export PATH=/usr/bin:/usr/sbin:/bin:/sbin:/home/base/tools:/home/app/localbin:/home/base:/tmp/sd/yi-hack-v5/bin:/tmp/sd/yi-hack-v5/sbin:/tmp/sd/yi-hack-v5/usr/bin:/tmp/sd/yi-hack-v5/usr/sbin
export LD_LIBRARY_PATH=/lib:/usr/lib:/home/lib:/home/qigan/lib:/home/app/locallib:/tmp/sd:/tmp/sd/gdb:/tmp/sd/yi-hack-v5/lib


NAME="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

if [ "$NAME" != "get" ] ; then
    exit
fi

if [ "$VAL" == "info" ] ; then
    printf "Content-type: application/json\r\n\r\n"

    FW_VERSION=`cat /tmp/sd/yi-hack-v5/version`
    LATEST_FW=`wget -O - https://api.github.com/repos/alienatedsec/yi-hack-v5/releases/latest 2>&1 | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/'`
    PRERELEASE_FW=`wget -O - https://api.github.com/repos/alienatedsec/yi-hack-v5/releases 2>&1 | grep -B 4 '"prerelease": true' | awk -F '"' '{print $4; exit}'`
	
    printf "{\n"
    printf "\"%s\":\"%s\",\n" "fw_version"       "$FW_VERSION"
    printf "\"%s\":\"%s\",\n" "latest_fw"       "$LATEST_FW"
    printf "\"%s\":\"%s\"\n" "prerelease_fw"       "$PRERELEASE_FW"
    printf "}"

elif [ "$VAL" == "upgrade" ] ; then
    FREE_SD=$(df /tmp/sd/ | grep mmc | awk '{print $4}')
    if [ -z "$FREE_SD" ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No SD detected."
        exit
    fi

    if [ $FREE_SD -lt 100000 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No space left on SD."
        exit
    fi

    # Clean old upgrades
    rm -rf /tmp/sd/${MODEL_SUFFIX}
    rm -rf /tmp/sd/${MODEL_SUFFIX}.conf
    rm -rf /tmp/sd/Factory
    rm -rf /tmp/sd/newhome
    rm /tmp/sd/rootfs*
    rm /tmp/sd/home*

    mkdir -p /tmp/sd/${MODEL_SUFFIX}
    mkdir -p /tmp/sd/${MODEL_SUFFIX}.conf
 #   cd /tmp/sd/${MODEL_SUFFIX}
    cd /tmp/sd
    
    if [ -f /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz ]; then
#        mv /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz /tmp/sd/${MODEL_SUFFIX}/${MODEL_SUFFIX}_x.x.x.tgz
        LATEST_FW="x.x.x"
    else
        LATEST_FW=`wget -O -  https://api.github.com/repos/alienatedsec/yi-hack-v5/releases/latest 2>&1 | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/'`
        if [ "$FW_VERSION" == "$LATEST_FW" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "No new firmware available."
            exit
        elif [ "$BASELINE_VERSION" != "0.4.1" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Wrong baseline version"
            exit
        fi
        
        wget https://github.com/alienatedsec/yi-hack-v5/releases/download/$LATEST_FW/${MODEL_SUFFIX}_${LATEST_FW}.tgz
        
        if [ ! -f ${MODEL_SUFFIX}_${LATEST_FW}.tgz ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Unable to download firmware file."
            exit
        fi
    fi

    # Backup configuration
    cp -rf $YI_HACK_PREFIX/etc/* /tmp/sd/${MODEL_SUFFIX}.conf/
    rm /tmp/sd/${MODEL_SUFFIX}.conf/*.tar.gz

    # Prepare new hack
    gzip -d ${MODEL_SUFFIX}_${LATEST_FW}.tgz
    tar xvf ${MODEL_SUFFIX}_${LATEST_FW}.tar
    rm ${MODEL_SUFFIX}_${LATEST_FW}.tar
    mkdir -p /tmp/sd/${MODEL_SUFFIX}/yi-hack-v5/etc
    cp -rf /tmp/sd/${MODEL_SUFFIX}.conf/* /tmp/sd/${MODEL_SUFFIX}/yi-hack-v5/etc/

    # Report the status to the caller
    printf "Content-type: text/html\r\n\r\n"
    printf "Download completed, rebooting and upgrading."

    sync
    sync
    sync
    sleep 1
    reboot

elif [ "$VAL" == "tgz" ] ; then
    # Manual install of an uploaded .tgz package.
    # The package is uploaded via upload.sh (stored as fw_upload.tgz
    # in the hack prefix) or placed on the SD root.
    TGAZ_FILE="$YI_HACK_PREFIX/fw_upload.tgz"
    if [ ! -f "$TGAZ_FILE" ]; then
        TGAZ_FILE="/tmp/sd/fw_upload.tgz"
    fi
    if [ ! -f "$TGAZ_FILE" ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No firmware package found. Upload it first."
        exit
    fi

    FREE_SD=$(df /tmp/sd/ | grep mmc | awk '{print $4}')
    if [ -z "$FREE_SD" ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No SD detected."
        exit
    fi

    if [ $FREE_SD -lt 100000 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No space left on SD."
        exit
    fi

    # Validate: must be a readable tgz containing this camera's hack tree.
    # (busybox tar here has no -z; tf - lists bare names, no -v)
    if ! gzip -dc "$TGAZ_FILE" 2>/dev/null | tar tf - 2>/dev/null | grep -q "^${MODEL_SUFFIX}/yi-hack-v5/version$"; then
        printf "Content-type: text/html\r\n\r\n"
        printf "Invalid firmware package (missing ${MODEL_SUFFIX}/yi-hack-v5 tree)."
        rm -f "$TGAZ_FILE"
        exit
    fi

    # Hand over to the standard local-package path.
    # Move it where the local-package branch expects it, then run that
    # branch detached so the install survives the HTTP client disconnecting
    # (extraction + reboot takes a few minutes).
    mv "$TGAZ_FILE" "/tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz"
    rm -f "$YI_HACK_PREFIX/fw_upload.tgz" /tmp/sd/fw_upload.tgz
    printf "Content-type: text/html\r\n\r\n"
    printf "Install started. The camera will reboot in a few seconds."
    nohup sh -c "env QUERY_STRING=get=upgrade sh $YI_HACK_PREFIX/www/cgi-bin/fw_upgrade.sh > /dev/null 2>&1" &
    exit

elif [ "$VAL" == "preupgrade" ] ; then
    FREE_SD=$(df /tmp/sd/ | grep mmc | awk '{print $4}')
    if [ -z "$FREE_SD" ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No SD detected."
        exit
    fi

    if [ $FREE_SD -lt 100000 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "No space left on SD."
        exit
    fi

    # Clean old upgrades
    rm -rf /tmp/sd/${MODEL_SUFFIX}
    rm -rf /tmp/sd/${MODEL_SUFFIX}.conf
    rm -rf /tmp/sd/Factory
    rm -rf /tmp/sd/newhome
    rm /tmp/sd/rootfs*
    rm /tmp/sd/home*

    mkdir -p /tmp/sd/${MODEL_SUFFIX}
    mkdir -p /tmp/sd/${MODEL_SUFFIX}.conf
 #   cd /tmp/sd/${MODEL_SUFFIX}
    cd /tmp/sd
    
    if [ -f /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz ]; then
#        mv /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz /tmp/sd/${MODEL_SUFFIX}/${MODEL_SUFFIX}_x.x.x.tgz
        PRERELEASE_FW="x.x.x"
    else
        PRERELEASE_FW=`wget -O - https://api.github.com/repos/alienatedsec/yi-hack-v5/releases 2>&1 | grep -B 4 '"prerelease": true' | awk -F '"' '{print $4; exit}'`
        if [ "$FW_VERSION" == "$PRERELEASE_FW" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "No new firmware available."
            exit
        elif [ "$BASELINE_VERSION" != "0.4.1" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Wrong baseline version"
            exit
        fi

        wget https://github.com/alienatedsec/yi-hack-v5/releases/download/$PRERELEASE_FW/${MODEL_SUFFIX}_${PRERELEASE_FW}.tgz
        
        if [ ! -f ${MODEL_SUFFIX}_${PRERELEASE_FW}.tgz ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Unable to download firmware file."
            exit
        fi
    fi

    # Backup configuration
    cp -rf $YI_HACK_PREFIX/etc/* /tmp/sd/${MODEL_SUFFIX}.conf/
    rm /tmp/sd/${MODEL_SUFFIX}.conf/*.tar.gz

    # Prepare new hack
    gzip -d ${MODEL_SUFFIX}_${PRERELEASE_FW}.tgz
    tar xvf ${MODEL_SUFFIX}_${PRERELEASE_FW}.tar
    rm ${MODEL_SUFFIX}_${PRERELEASE_FW}.tar
    mkdir -p /tmp/sd/${MODEL_SUFFIX}/yi-hack-v5/etc
    cp -rf /tmp/sd/${MODEL_SUFFIX}.conf/* /tmp/sd/${MODEL_SUFFIX}/yi-hack-v5/etc/

    # Report the status to the caller
    printf "Content-type: text/html\r\n\r\n"
    printf "Download completed, rebooting and upgrading."

    sync
    sync
    sync
    sleep 1
    reboot
fi

