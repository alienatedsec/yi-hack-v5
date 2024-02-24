#!/bin/sh

# 0.4.1

printf "Content-type: text/javascript\r\n\r\n"

printf "hostname=\"%s\";" $(cat /tmp/sd/yi-hack-v5/etc/hostname)
