#!/bin/sh

# 0.4.1

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
MODEL_SUFFIX=`cat /home/app/.camver`
FW_VERSION=`cat /tmp/sd/yi-hack-v5/version`
# Strip any pre-release letter suffix (e.g. 0.4.3b -> 0.4.3): a non-numeric
# patch field makes busybox printf emit 0 for it, so a 0.4.x pre-release
# baseline would be misread as 0.4.0 and fail the check below.
BASELINE_VERSION=$(echo `cat /home/yi-hack-v5/version 2>/dev/null` | sed 's/[^0-9.]//g')

# Flashed baseline (rootfs and home partitions) must be 0.4.1 or newer
BASELINE_MAJOR=$(echo $BASELINE_VERSION | cut -d'.' -f1)
BASELINE_MINOR=$(echo $BASELINE_VERSION | cut -d'.' -f2)
BASELINE_PATCH=$(echo $BASELINE_VERSION | cut -d'.' -f3)
BASELINE_NUM=$(printf "%03d%03d%03d" $BASELINE_MAJOR $BASELINE_MINOR $BASELINE_PATCH)
MIN_BASELINE_NUM=$(printf "%03d%03d%03d" 0 4 1)

export PATH=/usr/bin:/usr/sbin:/bin:/sbin:/home/base/tools:/home/app/localbin:/home/base:/tmp/sd/yi-hack-v5/bin:/tmp/sd/yi-hack-v5/sbin:/tmp/sd/yi-hack-v5/usr/bin:/tmp/sd/yi-hack-v5/usr/sbin
export LD_LIBRARY_PATH=/lib:/usr/lib:/home/lib:/home/qigan/lib:/home/app/locallib:/tmp/sd:/tmp/sd/gdb:/tmp/sd/yi-hack-v5/lib


NAME="$(echo $QUERY_STRING | cut -d'=' -f1)"
VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"

# Download a firmware package with resume, a read timeout and retries,
# then verify the result by fully decompressing it: gzip -dc validates
# the whole stream and its CRC, so a truncated or corrupt download is
# caught here, before any install is attempted. A failed attempt is
# deleted and retried; after the final failure the caller reports the
# error and the user can simply try again (a clean start every time).
# $1 = URL, $2 = output file. Returns 0 on success, 1 on failure.
download_pkg()
{
    local URL=$1 FILE=$2
    local i=0
    while [ $i -lt 3 ]; do
        # -c resumes a partial file left by an earlier interrupted
        # download (e.g. the camera rebooted mid-fetch); -T 60 aborts a
        # dead/stalled connection after 60 s of silence (the old bare
        # wget hung forever on a dropped link); -q keeps wget's progress
        # out of the HTTP response.
        wget -c -q -T 60 -O "$FILE" "$URL" 2>/dev/null
        if [ -s "$FILE" ] && gzip -dc "$FILE" > /dev/null 2>&1; then
            return 0
        fi
        rm -f "$FILE"
        i=$((i+1))
        [ $i -lt 3 ] && sleep 2
    done
    return 1
}

if [ "$NAME" != "get" ] ; then
    exit
fi

if [ "$VAL" == "info" ] ; then
    # The GitHub API allows 60 requests/hour per IP (shared with the
    # rest of the LAN on the same WAN IP), so the status page is served
    # from a 10-minute local cache. Only cache when the API actually
    # answered (LATEST_FW non-empty) so a rate-limited response is not
    # pinned for the whole TTL.
    #
    # This action must NEVER block: the status page fires it twice per
    # load, and the camera's httpd is single-threaded - a blocked info
    # request freezes every other request, including a firmware upload
    # (the browser then aborts the stalled transfer and a partial
    # fw_upload.tgz is left behind). So a stale or missing cache kicks
    # a detached refresher (get=refresh) and answers immediately with
    # the last known values, flagged "checking" so the page re-polls.
    CACHE_FILE="/tmp/fwup_info_cache"
    CACHE_TTL=600
    CHECK_FILE="/tmp/fwup_info_checking"
    NOW=$(date +%s)
    C_TS=""; C_FW=""; C_LATEST=""; C_PRE=""
    CHECKING="false"

    if [ -f "$CACHE_FILE" ] ; then
        read C_TS C_FW C_LATEST C_PRE < "$CACHE_FILE"
    fi

    if [ -n "$C_TS" ] && [ $((NOW - C_TS)) -lt $CACHE_TTL ] ; then
        # Fresh cache - served as is.
        :
    else
        CHECKING="true"
        if [ ! -f "$CHECK_FILE" ] ; then
            echo "$$" > "$CHECK_FILE"
            # NB: the redirections must be OUTSIDE the sh -c string. The
            # httpd only finishes the CGI response when the CGI's stdout
            # pipe gets EOF, i.e. when every process holding it exits -
            # a nohup that inherits the pipe (redirections inside the
            # string apply to its child, not to it) would hold the
            # response hostage for the whole duration of the fetch.
            nohup sh -c "env QUERY_STRING=get=refresh sh $YI_HACK_PREFIX/www/cgi-bin/fw_upgrade.sh" > /dev/null 2>&1 &
        fi
    fi

    printf "Content-type: application/json\r\n\r\n"
    printf "{\n"
    printf "\"%s\":\"%s\",\n" "fw_version"       "${C_FW:-$FW_VERSION}"
    printf "\"%s\":\"%s\",\n" "latest_fw"       "$C_LATEST"
    printf "\"%s\":\"%s\",\n" "prerelease_fw"   "$C_PRE"
    printf "\"%s\":%s\n" "checking" "$CHECKING"
    printf "}"

elif [ "$VAL" == "refresh" ] ; then
    # Detached cache refresher, re-invoked by get=info above through
    # nohup - it must never hold the httpd, that is the whole point of
    # the split. Runs 3 bounded attempts, writes the cache only on
    # success, and always clears the in-flight marker on exit.
    CHECK_FILE="/tmp/fwup_info_checking"
    trap 'rm -f "$CHECK_FILE"' EXIT
    CACHE_FILE="/tmp/fwup_info_cache"
    LATEST_FW=""
    PRERELEASE_FW=""
    i=0
    while [ $i -lt 3 ]; do
        # -T 30 aborts a dead/stalled WAN connection (a bare wget hangs
        # in the kernel TCP timeout for ~2 minutes per attempt); see
        # download_pkg above for why the stdout-pipe variant is avoided.
        wget -T 30 -q -O /tmp/fwup_releases.json https://api.github.com/repos/alienatedsec/yi-hack-v5/releases 2>/dev/null
        LATEST_FW=`jq -r 'map(select(.prerelease == false)) | first | .tag_name // empty' /tmp/fwup_releases.json 2>/dev/null`
        PRERELEASE_FW=`jq -r '[.[] | select(.prerelease == true)] | first | .tag_name // empty' /tmp/fwup_releases.json 2>/dev/null`
        [ -n "$LATEST_FW" ] && break
        rm -f /tmp/fwup_releases.json
        i=$((i+1))
        [ $i -lt 3 ] && sleep 2
    done
    rm -f /tmp/fwup_releases.json
    if [ -n "$LATEST_FW" ] ; then
        printf "%s %s %s %s" "$(date +%s)" "$FW_VERSION" "$LATEST_FW" "$PRERELEASE_FW" > "$CACHE_FILE"
    fi
    exit 0

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

    # The fetch + download + extraction + reboot takes several minutes.
    # Running it inside this browser request freezes the single-
    # threaded httpd the whole time, and the open connection is a live
    # wire: any page refresh closes it, the httpd dies, and the verbose
    # tar gets SIGPIPE mid-extraction - leaving a partial staging tree
    # (rootfs written, home missing) and no reboot. So a request that
    # is not already the detached install only re-dispatches it (the
    # tgz branch's proven pattern) with stdout on /dev/null and
    # answers immediately; the real work happens out of reach of the
    # browser.
    if [ -z "$YI_FW_DETACHED" ] ; then
        UP_MARK="/tmp/fwup_upgrading"
        if [ -f "$UP_MARK" ] ; then
            UP_TS=$(cat "$UP_MARK" 2>/dev/null)
            NOW=$(date +%s)
            if [ -n "$UP_TS" ] && [ $((NOW - UP_TS)) -lt 900 ] ; then
                printf "Content-type: text/html\r\n\r\n"
                printf "An upgrade is already in progress. The camera will reboot when it finishes."
                exit
            fi
        fi
        nohup sh -c "env YI_FW_DETACHED=1 QUERY_STRING=get=upgrade sh $YI_HACK_PREFIX/www/cgi-bin/fw_upgrade.sh" > /dev/null 2>&1 &
        printf "Content-type: text/html\r\n\r\n"
        printf "Install started. The camera will reboot in a few seconds."
        exit
    fi
    # The detached install owns an in-progress marker until it exits:
    # on failure the trap removes it (so a retry works right away), on
    # success the reboot wipes /tmp. It is re-stamped after the
    # download so a slow WAN fetch cannot age it out of the 900 s
    # window.
    UP_MARK="/tmp/fwup_upgrading"
    printf "%s" "$(date +%s)" > "$UP_MARK"
    trap 'rm -f "$UP_MARK"' EXIT

    # Clean old upgrades
    rm -rf /tmp/sd/${MODEL_SUFFIX}
    rm -rf /tmp/sd/${MODEL_SUFFIX}.conf
    rm -rf /tmp/sd/Factory
    rm -rf /tmp/sd/newhome
    rm /tmp/sd/rootfs*
    rm /tmp/sd/home*
    # Remove stale package files left behind by interrupted installs
    # (a partial run's tgz + tar can be 30 MB+ and fills the SD over
    # repeated failed upgrades). The manual x.x.x package is kept - it
    # is what this run installs if present.
    for f in /tmp/sd/${MODEL_SUFFIX}_*.tgz /tmp/sd/${MODEL_SUFFIX}_*.tar; do
        [ -f "$f" ] || continue
        case "$f" in *_x.x.x.*) continue;; esac
        rm -f "$f"
    done
    rm -f "$YI_HACK_PREFIX/fw_upload.tgz"

    mkdir -p /tmp/sd/${MODEL_SUFFIX}
    mkdir -p /tmp/sd/${MODEL_SUFFIX}.conf
 #   cd /tmp/sd/${MODEL_SUFFIX}
    cd /tmp/sd
    
    if [ -f /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz ]; then
#        mv /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz /tmp/sd/${MODEL_SUFFIX}/${MODEL_SUFFIX}_x.x.x.tgz
        LATEST_FW="x.x.x"
    else
        # File-based fetch with jq, like the preupgrade path: the stdout
        # pipe variant truncates on the camera and broke when GitHub
        # changed its response layout.
        LATEST_FW=""
        i=0
        while [ $i -lt 3 ]; do
            wget -O /tmp/fwup_latest.json https://api.github.com/repos/alienatedsec/yi-hack-v5/releases/latest 2>/dev/null
            LATEST_FW=`jq -r '.tag_name // empty' /tmp/fwup_latest.json 2>/dev/null`
            [ -n "$LATEST_FW" ] && break
            i=$((i+1))
        done
        rm -f /tmp/fwup_latest.json
        if [ -z "$LATEST_FW" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Unable to contact the firmware repository."
            exit
        fi
        # Numeric newness check (letter suffix stripped, same trick as
        # the baseline check at the top) so this never installs a
        # version equal to or older than the one running - e.g. a
        # 0.4.3d camera must not be offered the 0.4.2 "latest".
        LATEST_FW_CLEAN=$(echo $LATEST_FW | sed 's/[^0-9.]//g')
        LATEST_FW_NUM=$(printf "%03d%03d%03d" $(echo $LATEST_FW_CLEAN | cut -d'.' -f1) $(echo $LATEST_FW_CLEAN | cut -d'.' -f2) $(echo $LATEST_FW_CLEAN | cut -d'.' -f3))
        FW_VERSION_CLEAN=$(echo $FW_VERSION | sed 's/[^0-9.]//g')
        FW_VERSION_NUM=$(printf "%03d%03d%03d" $(echo $FW_VERSION_CLEAN | cut -d'.' -f1) $(echo $FW_VERSION_CLEAN | cut -d'.' -f2) $(echo $FW_VERSION_CLEAN | cut -d'.' -f3))
        if [ -z "$LATEST_FW_NUM" ] || [ -z "$FW_VERSION_NUM" ] || [ "$LATEST_FW_NUM" -le "$FW_VERSION_NUM" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "No new firmware available."
            exit
        elif [ "$BASELINE_NUM" -lt "$MIN_BASELINE_NUM" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Wrong baseline version"
            exit
        fi

        download_pkg "https://github.com/alienatedsec/yi-hack-v5/releases/download/$LATEST_FW/${MODEL_SUFFIX}_${LATEST_FW}.tgz" "/tmp/sd/${MODEL_SUFFIX}_${LATEST_FW}.tgz"
        if [ $? -ne 0 ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Unable to download firmware file."
            exit
        fi
        # The download is the slowest phase; re-stamp the in-progress
        # marker so it cannot age out while the extraction runs.
        printf "%s" "$(date +%s)" > "$UP_MARK"
    fi

    # Backup configuration
    cp -rf $YI_HACK_PREFIX/etc/* /tmp/sd/${MODEL_SUFFIX}.conf/
    rm /tmp/sd/${MODEL_SUFFIX}.conf/*.tar.gz

    # Prepare new hack. -dc + explicit redirect: the output name is
    # this script's, not the gzip applet's (plain -d names its output
    # after the input's suffix and that differs between busybox builds,
    # which is how an install can fail at "tar: no such file").
    gzip -dc ${MODEL_SUFFIX}_${LATEST_FW}.tgz > ${MODEL_SUFFIX}_${LATEST_FW}.tar
    if [ $? -ne 0 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "Package is corrupt (decompression failed). Remove it from /tmp/sd and retry."
        exit
    fi
    # No -v: the verbose listing is written to stdout, which the httpd
    # routes through a pipe the browser can close at any moment - a
    # SIGPIPE there kills the extraction mid-run and leaves a partial
    # staging tree (rootfs written, home missing, no reboot).
    tar xf ${MODEL_SUFFIX}_${LATEST_FW}.tar
    if [ $? -ne 0 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "Package is corrupt (extraction failed). Remove it from /tmp/sd and retry."
        exit
    fi
    rm ${MODEL_SUFFIX}_${LATEST_FW}.tar
    # Consume the manual x.x.x package (if this install used one), otherwise
    # the next upgrade click re-installs the stale file instead of checking
    # GitHub.
    rm -f /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz
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
    # (extraction + reboot takes a few minutes). YI_FW_DETACHED marks the
    # manual path as already detached (no re-dispatch), and the
    # redirections sit OUTSIDE the sh -c string - inside them they apply
    # to the child only, and this nohup would hold the response hostage
    # until the install finished.
    mv "$TGAZ_FILE" "/tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz"
    rm -f "$YI_HACK_PREFIX/fw_upload.tgz" /tmp/sd/fw_upload.tgz
    printf "Content-type: text/html\r\n\r\n"
    printf "Install started. The camera will reboot in a few seconds."
    nohup sh -c "env YI_FW_DETACHED=1 QUERY_STRING=get=upgrade sh $YI_HACK_PREFIX/www/cgi-bin/fw_upgrade.sh" > /dev/null 2>&1 &
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

    # Same detach as the upgrade branch: the install runs out of reach
    # of the browser connection, so a page refresh mid-install cannot
    # kill it.
    if [ -z "$YI_FW_DETACHED" ] ; then
        UP_MARK="/tmp/fwup_upgrading"
        if [ -f "$UP_MARK" ] ; then
            UP_TS=$(cat "$UP_MARK" 2>/dev/null)
            NOW=$(date +%s)
            if [ -n "$UP_TS" ] && [ $((NOW - UP_TS)) -lt 900 ] ; then
                printf "Content-type: text/html\r\n\r\n"
                printf "An upgrade is already in progress. The camera will reboot when it finishes."
                exit
            fi
        fi
        nohup sh -c "env YI_FW_DETACHED=1 QUERY_STRING=get=preupgrade sh $YI_HACK_PREFIX/www/cgi-bin/fw_upgrade.sh" > /dev/null 2>&1 &
        printf "Content-type: text/html\r\n\r\n"
        printf "Install started. The camera will reboot in a few seconds."
        exit
    fi
    UP_MARK="/tmp/fwup_upgrading"
    printf "%s" "$(date +%s)" > "$UP_MARK"
    trap 'rm -f "$UP_MARK"' EXIT

    # Clean old upgrades
    rm -rf /tmp/sd/${MODEL_SUFFIX}
    rm -rf /tmp/sd/${MODEL_SUFFIX}.conf
    rm -rf /tmp/sd/Factory
    rm -rf /tmp/sd/newhome
    rm /tmp/sd/rootfs*
    rm /tmp/sd/home*
    # Remove stale package files left behind by interrupted installs
    # (a partial run's tgz + tar can be 30 MB+ and fills the SD over
    # repeated failed upgrades). The manual x.x.x package is kept - it
    # is what this run installs if present.
    for f in /tmp/sd/${MODEL_SUFFIX}_*.tgz /tmp/sd/${MODEL_SUFFIX}_*.tar; do
        [ -f "$f" ] || continue
        case "$f" in *_x.x.x.*) continue;; esac
        rm -f "$f"
    done
    rm -f "$YI_HACK_PREFIX/fw_upload.tgz"

    mkdir -p /tmp/sd/${MODEL_SUFFIX}
    mkdir -p /tmp/sd/${MODEL_SUFFIX}.conf
 #   cd /tmp/sd/${MODEL_SUFFIX}
    cd /tmp/sd
    
    if [ -f /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz ]; then
#        mv /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz /tmp/sd/${MODEL_SUFFIX}/${MODEL_SUFFIX}_x.x.x.tgz
        PRERELEASE_FW="x.x.x"
    else
        # File-based fetch with retries: see the info action above for why
        # the stdout-pipe variant truncates on the camera.
        PRERELEASE_FW=""
        i=0
        while [ $i -lt 3 ]; do
            wget -O /tmp/fwup_releases.json https://api.github.com/repos/alienatedsec/yi-hack-v5/releases 2>/dev/null
            PRERELEASE_FW=`jq -r '[.[] | select(.prerelease == true)] | first | .tag_name // empty' /tmp/fwup_releases.json 2>/dev/null`
            [ -n "$PRERELEASE_FW" ] && break
            i=$((i+1))
        done
        rm -f /tmp/fwup_releases.json
        if [ -z "$PRERELEASE_FW" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "No pre-release available."
            exit
        fi
        # Numeric newness check (letter suffix stripped, same trick as
        # the baseline check at the top) so this never installs a
        # version equal to or older than the one running.
        PRERELEASE_FW_CLEAN=$(echo $PRERELEASE_FW | sed 's/[^0-9.]//g')
        PRERELEASE_FW_NUM=$(printf "%03d%03d%03d" $(echo $PRERELEASE_FW_CLEAN | cut -d'.' -f1) $(echo $PRERELEASE_FW_CLEAN | cut -d'.' -f2) $(echo $PRERELEASE_FW_CLEAN | cut -d'.' -f3))
        FW_VERSION_CLEAN=$(echo $FW_VERSION | sed 's/[^0-9.]//g')
        FW_VERSION_NUM=$(printf "%03d%03d%03d" $(echo $FW_VERSION_CLEAN | cut -d'.' -f1) $(echo $FW_VERSION_CLEAN | cut -d'.' -f2) $(echo $FW_VERSION_CLEAN | cut -d'.' -f3))
        if [ -z "$PRERELEASE_FW_NUM" ] || [ -z "$FW_VERSION_NUM" ] || [ "$PRERELEASE_FW_NUM" -le "$FW_VERSION_NUM" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "No new firmware available."
            exit
        elif [ "$BASELINE_NUM" -lt "$MIN_BASELINE_NUM" ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Wrong baseline version"
            exit
        fi

        download_pkg "https://github.com/alienatedsec/yi-hack-v5/releases/download/$PRERELEASE_FW/${MODEL_SUFFIX}_${PRERELEASE_FW}.tgz" "/tmp/sd/${MODEL_SUFFIX}_${PRERELEASE_FW}.tgz"
        if [ $? -ne 0 ]; then
            printf "Content-type: text/html\r\n\r\n"
            printf "Unable to download firmware file."
            exit
        fi
        # The download is the slowest phase; re-stamp the in-progress
        # marker so it cannot age out while the extraction runs.
        printf "%s" "$(date +%s)" > "$UP_MARK"
    fi

    # Backup configuration
    cp -rf $YI_HACK_PREFIX/etc/* /tmp/sd/${MODEL_SUFFIX}.conf/
    rm /tmp/sd/${MODEL_SUFFIX}.conf/*.tar.gz

    # Prepare new hack. -dc + explicit redirect: the output name is
    # this script's, not the gzip applet's (plain -d names its output
    # after the input's suffix and that differs between busybox builds,
    # which is how an install can fail at "tar: no such file").
    gzip -dc ${MODEL_SUFFIX}_${PRERELEASE_FW}.tgz > ${MODEL_SUFFIX}_${PRERELEASE_FW}.tar
    if [ $? -ne 0 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "Package is corrupt (decompression failed). Remove it from /tmp/sd and retry."
        exit
    fi
    # No -v: the verbose listing is written to stdout, which the httpd
    # routes through a pipe the browser can close at any moment - a
    # SIGPIPE there kills the extraction mid-run and leaves a partial
    # staging tree (rootfs written, home missing, no reboot).
    tar xf ${MODEL_SUFFIX}_${PRERELEASE_FW}.tar
    if [ $? -ne 0 ]; then
        printf "Content-type: text/html\r\n\r\n"
        printf "Package is corrupt (extraction failed). Remove it from /tmp/sd and retry."
        exit
    fi
    rm ${MODEL_SUFFIX}_${PRERELEASE_FW}.tar
    # Consume the manual x.x.x package (if this install used one), otherwise
    # the next upgrade click re-installs the stale file instead of checking
    # GitHub.
    rm -f /tmp/sd/${MODEL_SUFFIX}_x.x.x.tgz
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

