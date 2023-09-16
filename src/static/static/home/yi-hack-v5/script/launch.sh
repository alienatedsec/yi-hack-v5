#!/bin/sh
#
# 0.4.1b
#
#  This file is part of yi-hack-v5 (https://github.com/alienatedsec/yi-hack-v5).
#  Copyright (c) 2021-2023 alienatedsec - v5 specific
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, version 3.
# 
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <http://www.gnu.org/licenses/>.
#


kill_all() {
  # Kills all running instances
  killall -s SIGTERM wd_rtsp.sh
  killall -s SIGTERM rRTSPServer
  killall -s SIGTERM h264grabber
  
  sleep 10
  
  killall -s SIGKILL wd_rtsp.sh
  killall -s SIGKILL rRTSPServer
  killall -s SIGKILL h264grabber
}

launch_apps() {
  # Launches apps with different commands
  if [ "$execute_audio" = true ]; then
    h264grabber -r audio -m "$model" -f &
  fi
  
  if [ "$settings" = "high" ]; then
    debug_flag=""
    if [ "$debug_mode" = true ]; then
      debug_flag="-d 7"
    fi
    h264grabber -r high -m "$model" -f &
    rRTSPServer -r high -a "$enable_audio" -p 554 -u -w $debug_flag &
  fi

  if [ "$settings" = "low" ]; then
    debug_flag=""
    if [ "$debug_mode" = true ]; then
      debug_flag="-d 7"
    fi
    h264grabber -r low -m "$model" -f &
    rRTSPServer -r low -a "$enable_audio" -p 554 -u -w $debug_flag &
  fi
  
  if [ "$settings" = "both" ]; then
    debug_flag=""
    if [ "$debug_mode" = true ]; then
      debug_flag="-d 7"
    fi
    h264grabber -r low -m "$model" -f &
	h264grabber -r high -m "$model" -f &
    rRTSPServer -r both -a "$enable_audio" -p 554 -u -w $debug_flag &
  fi

}


# Read model and versions from files
model=$(cat /home/app/.camver)
version_cam=$(cat /home/yi-hack-v5/version)
version_sd=$(cat /tmp/sd/yi-hack-v5/version)
echo "Using model: $model"
echo "Version on cam: $version_cam"
echo "Version on sd: $version_sd"

read -p "Specify the operation (k for kill, l for launch): " operation

if [ "$operation" = "k" ] || [ "$operation" = "K" ]; then
  kill_all
elif [ "$operation" = "l" ] || [ "$operation" = "L" ]; then
  read -p "Specify the settings (high, low, or both): " settings
  read -p "Enable audio? (y/n): " audio_choice

  if [ "$audio_choice" = "y" ] || [ "$audio_choice" = "Y" ]; then
    execute_audio=true
    enable_audio="yes"
  else
    execute_audio=false
    enable_audio="no"
  fi

  read -p "Execute in debug mode? (y/n): " debug_choice

  if [ "$debug_choice" = "y" ] || [ "$debug_choice" = "Y" ]; then
    debug_mode=true
  else
    debug_mode=false
  fi

  launch_apps
else
  echo "Invalid operation."
fi
