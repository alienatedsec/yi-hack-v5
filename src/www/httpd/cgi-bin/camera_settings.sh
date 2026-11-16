#!/bin/sh

# 0.4.1

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"
CONF_FILE="$YI_HACK_PREFIX/etc/camera.conf"

CONF_LAST="CONF_LAST"

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
    elif [ "$CONF" == "save_video_on_motion" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -v always
        else
            ipc_cmd -v detect
        fi
    elif [ "$CONF" == "sensitivity" ] ; then
        ipc_cmd -s $VAL
    elif [ "$CONF" == "sound_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -b off
        else
            ipc_cmd -b on
        fi
    elif [ "$CONF" == "sound_sensitivity" ] ; then
        if [ "$VAL" == "50" ] || [ "$VAL" == "60" ] || [ "$VAL" == "70" ] || [ "$VAL" == "80" ] || [ "$VAL" == "90" ] ; then
            ipc_cmd -n $VAL
        fi
    elif [ "$CONF" == "baby_crying_detect" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -B off
        else
            ipc_cmd -B on
        fi
    elif [ "$CONF" == "led" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -l off
        else
            ipc_cmd -l on
        fi
    elif [ "$CONF" == "ir" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -i off
        else
            ipc_cmd -i on
        fi
    elif [ "$CONF" == "rotate" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -r off
        else
            ipc_cmd -r on
        fi
    elif [ "$CONF" == "ai_human_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -a off
        else
            ipc_cmd -a on
        fi
    elif [ "$CONF" == "ai_vehicle_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -E off
        else
            ipc_cmd -E on
        fi
    elif [ "$CONF" == "ai_animal_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -N off
        else
            ipc_cmd -N on
        fi
    elif [ "$CONF" == "motion_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -O off
        else
            ipc_cmd -O on
        fi
    elif [ "$CONF" == "face_detection" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -c off
        else
            ipc_cmd -c on
        fi
    elif [ "$CONF" == "mic" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -I off
        else
            ipc_cmd -I on
        fi
    elif [ "$CONF" == "motion_tracking" ] ; then
        if [ "$VAL" == "no" ] ; then
            ipc_cmd -o off
        else
            ipc_cmd -o on
        fi
    elif [ "$CONF" == "cruise" ] ; then
        if [ "$VAL" == "no" ] || [ "$VAL" == "off" ] ; then
            ipc_cmd -C off
        elif [ "$VAL" == "presets" ] || [ "$VAL" == "360" ] ; then
            ipc_cmd -C $VAL
        fi
    fi
    sleep 1
done

printf "Content-type: application/json\r\n\r\n"

printf "{\n"
printf "\"%s\":\"%s\"\\n" "error" "false"
printf "}"
