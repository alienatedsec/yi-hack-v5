#!/bin/sh

# 0.4.1

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
CONF_FILE="$YI_HACK_PREFIX/etc/camera.conf"

CONF_LAST="CONF_LAST"

# Persist a UI setting in the conf file so the page remembers the choice
# across reloads: update the existing line in place, or append the key when
# it is missing from an older conf (check_conf.sh only backfills at boot).
set_conf()
{
    if grep -q "^$1=" "$CONF_FILE" 2>/dev/null; then
        sed -i "s/^\($1[[:blank:]]*=[[:blank:]]*\).*$/\1$2/" "$CONF_FILE"
    else
        printf "%s=%s\n" "$1" "$2" >> "$CONF_FILE"
    fi
}

for I in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17
do
    CONF="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'&' -f$I | cut -d'=' -f2)"

    if [ $CONF == $CONF_LAST ]; then
        continue
    fi
    CONF_LAST=$CONF

    if [ "$CONF" == "switch_on" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -t off
            sleep 1
            ipc_cmd -T  # Stop current motion detection event
        else
            ipc_cmd -t on
        fi
        set_conf SWITCH_ON "$VAL"
    elif [ "$CONF" == "save_video_on_motion" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -v always
        else
            ipc_cmd -v detect
        fi
        set_conf SAVE_VIDEO_ON_MOTION "$VAL"
    elif [ "$CONF" == "sensitivity" ] ; then
        ipc_cmd -s $VAL
        set_conf SENSITIVITY "$VAL"
    elif [ "$CONF" == "sound_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -b off
        else
            ipc_cmd -b on
        fi
        set_conf SOUND_DETECTION "$VAL"
    elif [ "$CONF" == "sound_sensitivity" ] ; then
        if [ "$VAL" == "50" ] || [ "$VAL" == "60" ] || [ "$VAL" == "70" ] || [ "$VAL" == "80" ] || [ "$VAL" == "90" ] ; then
            ipc_cmd -n $VAL
            set_conf SOUND_SENSITIVITY "$VAL"
        fi
    elif [ "$CONF" == "baby_crying_detect" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -B off
        else
            ipc_cmd -B on
        fi
        set_conf BABY_CRYING_DETECT "$VAL"
    elif [ "$CONF" == "led" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -l off
        else
            ipc_cmd -l on
        fi
        set_conf LED "$VAL"
    elif [ "$CONF" == "ir" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -i off
        else
            ipc_cmd -i on
        fi
        set_conf IR "$VAL"
    elif [ "$CONF" == "rotate" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -r off
        else
            ipc_cmd -r on
        fi
        set_conf ROTATE "$VAL"
    elif [ "$CONF" == "ai_human_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -a off
        else
            ipc_cmd -a on
        fi
        set_conf AI_HUMAN_DETECTION "$VAL"
    elif [ "$CONF" == "ai_vehicle_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -E off
        else
            ipc_cmd -E on
        fi
        set_conf AI_VEHICLE_DETECTION "$VAL"
    elif [ "$CONF" == "ai_animal_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -N off
        else
            ipc_cmd -N on
        fi
        set_conf AI_ANIMAL_DETECTION "$VAL"
    elif [ "$CONF" == "motion_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -O off
        else
            ipc_cmd -O on
        fi
        set_conf MOTION_DETECTION "$VAL"
    elif [ "$CONF" == "face_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -c off
        else
            ipc_cmd -c on
        fi
        set_conf FACE_DETECTION "$VAL"
    elif [ "$CONF" == "mic" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -I off
        else
            ipc_cmd -I on
        fi
        set_conf MIC "$VAL"
    elif [ "$CONF" == "motion_tracking" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -o off
        else
            ipc_cmd -o on
        fi
        set_conf MOTION_TRACKING "$VAL"
    elif [ "$CONF" == "cruise" ] ; then
        if [ "$VAL" == "no" ] || [ "$VAL" == "off" ] ; then
            ipc_cmd -C off
            set_conf CRUISE "$VAL"
        elif [ "$VAL" == "presets" ] || [ "$VAL" == "360" ] ; then
            ipc_cmd -C $VAL
            set_conf CRUISE "$VAL"
        fi
    fi
    sleep 1
done

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
