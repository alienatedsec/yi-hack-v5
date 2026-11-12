#!/bin/sh

# wifi_connect.sh - Multi-WiFi connection manager for yi-hack-v5 (slim)
#
# Kills the stock single-network wpa_supplicant and replaces it with
# one that reads a multi-network wpa_supplicant.conf from the SD card.
# wpa_supplicant natively handles failover between configured networks
# using priority-based selection.

WPA_CONF="/tmp/sd/yi-hack-v5/etc/wpa_supplicant.conf"
WPA_BIN="/home/base/tools/wpa_supplicant"
WPA_LOG="/tmp/sd/wifi_multi.log"
WLAN_IF="wlan0"

log()
{
    echo "$(date +'%Y-%m-%d %H:%M:%S') - $1" >> $WPA_LOG
}

# Validate config file exists and has at least one network block
if [ ! -f "$WPA_CONF" ]; then
    log "ERROR: $WPA_CONF not found, aborting multi-WiFi"
    exit 1
fi

NETWORK_COUNT=$(grep -c "^network=" "$WPA_CONF" 2>/dev/null)
if [ "$NETWORK_COUNT" -eq 0 ] 2>/dev/null; then
    log "ERROR: No network blocks found in $WPA_CONF, aborting"
    exit 1
fi

log "Starting multi-WiFi connection manager ($NETWORK_COUNT networks configured)"

# Wait a moment for the stock wpa_supplicant to fully initialize
# (system.sh runs after stock firmware init, so it should be up already)
sleep 2

# Kill the stock wpa_supplicant (which reads single SSID from mtdblock2)
log "Stopping stock wpa_supplicant..."
killall wpa_supplicant 2>/dev/null
sleep 1

# Ensure wlan0 is up
ifconfig $WLAN_IF up 2>/dev/null

# Ensure the ctrl_interface directory exists
mkdir -p /var/run/wpa_supplicant 2>/dev/null

# Start wpa_supplicant with the multi-network config file
# -B = background (daemonize)
# -D wext = use the WEXT driver (standard for Yi cameras)
# -i wlan0 = interface
# -c = config file path
log "Starting wpa_supplicant with multi-network config..."
$WPA_BIN -B -D wext -i $WLAN_IF -c $WPA_CONF 2>>$WPA_LOG

if [ $? -ne 0 ]; then
    log "ERROR: wpa_supplicant failed to start with multi-network config"
    log "Attempting to restart stock wpa_supplicant as fallback..."
    # Restart stock wpa_supplicant by triggering the original wifi connection
    # The stock firmware reads from mtdblock2, so we just restart wpa_supplicant
    # without a config file (it will use the built-in mtdblock2 mechanism)
    $WPA_BIN -B -D wext -i $WLAN_IF 2>>$WPA_LOG
    exit 1
fi

# Wait for wpa_supplicant to associate with a network
WAIT_LIMIT=30
WAIT_COUNT=0
CONNECTED=0

while [ $WAIT_COUNT -lt $WAIT_LIMIT ]; do
    # Check if we are associated with any network
    ESSID=$(iwconfig $WLAN_IF 2>/dev/null | grep ESSID | sed 's/.*ESSID:"\(.*\)".*/\1/')
    if [ ! -z "$ESSID" ] && [ "$ESSID" != "off/any" ]; then
        CONNECTED=1
        break
    fi
    sleep 1
    WAIT_COUNT=$((WAIT_COUNT + 1))
done

if [ $CONNECTED -eq 1 ]; then
    log "Connected to WiFi network: $ESSID (after ${WAIT_COUNT}s)"

    # Restart DHCP to get an IP address on the new connection
    killall udhcpc 2>/dev/null
    sleep 1

    HN="yi-hack-v5"
    if [ -f /tmp/sd/yi-hack-v5/etc/hostname ]; then
        HN=$(cat /tmp/sd/yi-hack-v5/etc/hostname)
    fi
    /sbin/udhcpc -i $WLAN_IF -b -s /home/app/script/default.script -x hostname:$HN

    # Wait a moment for DHCP
    sleep 3
    IP_ADDR=$(ifconfig $WLAN_IF 2>/dev/null | grep "inet addr" | awk -F: '{print $2}' | awk '{print $1}')
    log "IP address: $IP_ADDR"
else
    log "WARNING: Failed to connect to any configured network within ${WAIT_LIMIT}s"
    log "wpa_supplicant will continue trying in the background"
fi
