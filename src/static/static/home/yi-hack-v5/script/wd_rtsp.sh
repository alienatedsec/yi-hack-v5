#!/bin/sh

# 0.4.1

script_name=$(basename -- "$0")

if pidof "$script_name" -o $$ >/dev/null;then
   echo "Already Running - Quitting"
   exit 1
fi

CONF_FILE="etc/system.conf"

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
MODEL_SUFFIX=$(cat /home/app/.camver)

LOG_FILE="/tmp/sd/wd_rtsp.log"
#LOG_FILE="/dev/null"

get_config()
{
    key=$1
    grep -w $1 $YI_HACK_PREFIX/$CONF_FILE | cut -d "=" -f2 | awk 'NR==1 {print; exit}'
}

COUNTER=0
COUNTER_LIMIT=10
INTERVAL=10

if [[ "$(get_config USERNAME)" != "" ]] ; then
    USERNAME=$(get_config USERNAME)
    PASSWORD=$(get_config PASSWORD)
fi

RRTSP_RES=$(get_config RTSP_STREAM)
RRTSP_AUDIO=$(get_config RTSP_AUDIO)
RRTSP_MODEL=$MODEL_SUFFIX
RRTSP_PORT=$(get_config RTSP_PORT)
# RTSP port as /proc/net/tcp prints it
RRTSP_PORT_HEX=$(printf '%04X' "$RRTSP_PORT" 2>/dev/null)
if [ ! -z $USERNAME ]; then
    RRTSP_USER="-u $USERNAME"
fi
if [ ! -z $PASSWORD ]; then
    RRTSP_PWD="-w $PASSWORD"
fi

# h264grabber processes restart_grabber() starts for this config
case $RRTSP_RES in
    both) GRABBER_COUNT=2 ;;
    low|high) GRABBER_COUNT=1 ;;
    *) GRABBER_COUNT=0 ;;
esac
if [[ "$RRTSP_AUDIO" == "yes" ]]; then
    GRABBER_COUNT=$((GRABBER_COUNT+1))
fi

restart_rtsp()
{
    killall -q rRTSPServer
    rRTSPServer -r $RRTSP_RES -a $RRTSP_AUDIO -p $RRTSP_PORT $RRTSP_USER $RRTSP_PWD &
}

restart_grabber()
{
    killall -q rRTSPServer
    killall -q h264grabber
    if [[ $(get_config RTSP_STREAM) == "low" ]]; then
        h264grabber -r low -m $MODEL_SUFFIX -f &
    fi
    if [[ $(get_config RTSP_STREAM) == "high" ]]; then
        h264grabber -r high -m $MODEL_SUFFIX -f &
    fi
    if [[ $(get_config RTSP_STREAM) == "both" ]]; then
        h264grabber -r low -m $MODEL_SUFFIX -f &
        h264grabber -r high -m $MODEL_SUFFIX -f &
    fi
    if [[ $(get_config RTSP_AUDIO) == "yes" ]]; then
        h264grabber -r AUDIO -m $MODEL_SUFFIX -f &
    fi
    rRTSPServer -r $RRTSP_RES -a $RRTSP_AUDIO -p $RRTSP_PORT $RRTSP_USER $RRTSP_PWD &
}

restart_cloud()
{
    if [[ $(get_config DISABLE_CLOUD) == "yes" ]] ; then
    (
        cd /home/app
        ./cloud &
    )
    fi
}

restart_mqttv4()
{
    mqttv4 &
}

# The checks below run every 10 s, so they read /proc with shell builtins
# instead of starting ps, top, netstat, grep and awk every time. Results are
# returned in variables, because $(...) forks in ash even for a function.

# read_conf KEY -> CONF_VAL (the value of KEY in system.conf)
read_conf()
{
    CONF_VAL=""
    while IFS='=' read -r K V || [ -n "$K" ]; do
        if [ "$K" == "$1" ]; then
            CONF_VAL=$V
            return 0
        fi
    done < $YI_HACK_PREFIX/$CONF_FILE
    return 1
}

# One pass over /proc: PID_RTSP ("" = rRTSPServer is not running) and the
# number of h264grabber, rmm, cloud and mqttv4 processes
scan_processes()
{
    PID_RTSP=""
    N_GRABBER=0
    N_RMM=0
    N_CLOUD=0
    N_MQTTV4=0
    for P in /proc/[0-9]*; do
        { read -r C < $P/comm; } 2>/dev/null || continue
        case "$C" in
            rRTSPServer) PID_RTSP=${P#/proc/} ;;
            h264grabber) N_GRABBER=$((N_GRABBER+1)) ;;
            rmm) N_RMM=$((N_RMM+1)) ;;
            cloud) N_CLOUD=$((N_CLOUD+1)) ;;
            mqttv4) N_MQTTV4=$((N_MQTTV4+1)) ;;
        esac
    done
}

# SOCKET: established TCP connections on the RTSP port
count_rtsp_sockets()
{
    SOCKET=0
    for F in /proc/net/tcp /proc/net/tcp6; do
        [ -r $F ] || continue
        while read -r SL LOCAL REMOTE STATE REST; do
            case "$LOCAL" in
                *:$RRTSP_PORT_HEX) [ "$STATE" == "01" ] && SOCKET=$((SOCKET+1)) ;;
            esac
        done < $F
    done
}

# TICKS: CPU time (utime + stime) used so far by process $1, "" if it is gone
proc_ticks()
{
    TICKS=""
    { read -r X X X X X X X X X X X X X U S X < /proc/$1/stat; } 2>/dev/null && TICKS=$((U + S))
}

check_rtsp()
{
    [ "$RTSP_RESTARTED" == "yes" ] && return
    count_rtsp_sockets
    # CPU: "" if rRTSPServer is not running, otherwise the CPU ticks it used
    # in the sample time. As before, this matters only while a client is
    # connected, so it is measured only then. The first sample takes 1 s, so
    # a server that works in bursts is not suspected. While counting, 0.3 s
    # is enough (one tick in any of the samples clears the count) and the
    # restart of a locked server comes about as fast as it did with top.
    CPU=""
    if [ -n "$PID_RTSP" ]; then
        CPU="-"
        if [ $SOCKET -gt 0 ]; then
            proc_ticks $PID_RTSP
            T1=$TICKS
            if [ $COUNTER -eq 0 ]; then
                sleep 1
            else
                sleep 0.3 2>/dev/null || sleep 1
            fi
            CPU_SAMPLED=yes
            proc_ticks $PID_RTSP
            if [ -n "$T1" ] && [ -n "$TICKS" ]; then
                CPU=$((TICKS - T1))
            else
                CPU=""
            fi
        fi
    fi

    if [ "$CPU" == "" ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting rRTSPServer ..." >> $LOG_FILE
        killall -q rRTSPServer
        sleep 1
        restart_rtsp
        COUNTER=0
    fi
    if [ $SOCKET -gt 0 ]; then
        if [ "$CPU" == "0" ]; then
            COUNTER=$((COUNTER+1))
            echo "$(date +'%Y-%m-%d %H:%M:%S') - Detected possible locked process ($COUNTER)" >> $LOG_FILE
            if [ $COUNTER -ge $COUNTER_LIMIT ]; then
                echo "$(date +'%Y-%m-%d %H:%M:%S') - Restarting rtsp process" >> $LOG_FILE
                killall -q rRTSPServer
                sleep 1
                restart_rtsp
                COUNTER=0
           fi
        else
            COUNTER=0
        fi
    fi
}

check_cloud()
{
    read_conf DISABLE_CLOUD
    if [[ "$CONF_VAL" == "yes" ]] ; then
        if [ $N_CLOUD -eq 0 ]; then
            echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting ./cloud & ..." >> $LOG_FILE
            restart_cloud
        fi
    fi
}

check_rmm()
{
    if [ $N_RMM -eq 0 ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - ./rmm is not running, restarting the camera  ..." >> $LOG_FILE
        reboot
    fi
}

check_grabber()
{
    if [ $N_GRABBER -lt $GRABBER_COUNT ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - $N_GRABBER of $GRABBER_COUNT h264grabber processes running, restarting h264grabber ..." >> $LOG_FILE
        killall -q h264grabber
        sleep 1
        restart_grabber
        # restart_grabber has just started a new rRTSPServer: the scan of
        # this loop is stale for it, so check_rtsp waits for the next loop
        RTSP_RESTARTED=yes
    fi
}

check_mqttv4()
{
    if [[ $(get_config MQTT) != "yes" ]] ; then
        return
    fi

    if [ $N_MQTTV4 -eq 0 ]; then
        echo "$(date +'%Y-%m-%d %H:%M:%S') - No running processes, restarting mqttv4 ..." >> $LOG_FILE
        killall -q mqttv4
        sleep 1
        restart_mqttv4
    fi
}

if [[ $(get_config RTSP) == "no" ]] ; then
    exit
fi

# Re-enabled when its starting
echo "$(date +'%Y-%m-%d %H:%M:%S') - Starting RTSP watchdog..." >> $LOG_FILE

while true
do
    scan_processes
    CPU_SAMPLED=no
    RTSP_RESTARTED=no
    check_grabber
    check_rtsp
    check_rmm
    check_cloud
    check_mqttv4
    if [ $COUNTER -eq 0 ]; then
        sleep $INTERVAL
    elif [ "$CPU_SAMPLED" != "yes" ]; then
        # Counting a possibly locked rRTSPServer: loop again soon, but not
        # without a pause. top used to take this time; the CPU sample takes
        # it while a client is connected, this covers the loops without one.
        sleep 1
    fi
done
